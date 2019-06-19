#include "wrappers/MinizipWrapper.h"
#include "miniz.h"
#include "minizip/mz_compat.h"
#ifdef _WIN32
#include <windows.h>
#endif

#define WRITEBUFFERSIZE (16 * 1024)

namespace modio
{
namespace minizipwrapper
{
void extract(std::string zip_path, std::string directory_path)
{
  directory_path = addSlashIfNeeded(directory_path);
  
  writeLogLine(std::string("Extracting ") + zip_path, MODIO_DEBUGLEVEL_LOG);
  unzFile zipfile = unzOpen(zip_path.c_str());

  if (zipfile == NULL)
  {
    writeLogLine("Cannot open " + zip_path, MODIO_DEBUGLEVEL_ERROR);
    return;
  }

  unz_global_info global_info;
  unzGetGlobalInfo(zipfile, &global_info);
  char read_buffer[READ_SIZE];

  uLong i;
  for (i = 0; i < global_info.number_entry; ++i)
  {
    unz_file_info file_info;
    char filename[MAX_FILENAME];
    char final_filename[MAX_FILENAME];

    int err = unzGetCurrentFileInfo(
        zipfile,
        &file_info,
        filename,
        MAX_FILENAME,
        NULL, 0, NULL, 0);

    if (err != UNZ_OK)
    {
      unzClose(zipfile);
      writeLogLine("error " + toString(err) + " with zipfile in unzGetCurrentFileInfo", MODIO_DEBUGLEVEL_ERROR);
      return;
    }

    strcpy(final_filename, directory_path.c_str());
    strcat(final_filename, filename);

    const size_t filename_length = strlen(filename);
    modio::createPath(directory_path + filename);
    
    if (filename[filename_length - 1] == dir_delimter)
    {
      createDirectory(final_filename);
    }
    else
    {
      err = unzOpenCurrentFile(zipfile);

      if (err != UNZ_OK)
      {
        writeLogLine(std::string("Cannot open ") + filename, MODIO_DEBUGLEVEL_ERROR);
        return;
      }

      std::string new_file_path = filename;
      FILE *out;
      out = fopen(final_filename, "wb");

      if (!out)
      {
        writeLogLine(std::string("error opening ") + final_filename, MODIO_DEBUGLEVEL_ERROR);
        return;
      }

      err = UNZ_OK;
      do
      {
        err = unzReadCurrentFile(zipfile, read_buffer, READ_SIZE);
        if (err < 0)
        {
          writeLogLine("error " + toString(err) + " with zipfile in unzReadCurrentFile", MODIO_DEBUGLEVEL_ERROR);
          unzCloseCurrentFile(zipfile);
          unzClose(zipfile);
          return;
        }
        if (err > 0)
        {
          if (fwrite(read_buffer, (size_t)err, 1, out) != 1)
          {
            writeLogLine("error " + toString(err) + " in writing extracted file", MODIO_DEBUGLEVEL_ERROR);
          }
        }
      } while (err > 0);

      fclose(out);

      err = unzCloseCurrentFile(zipfile);
      if (err != UNZ_OK)
        writeLogLine("error " + toString(err) + " with " + filename + " in unzCloseCurrentFile", MODIO_DEBUGLEVEL_ERROR);
    }

    if ((i + 1) < global_info.number_entry)
    {
      err = unzGoToNextFile(zipfile);

      if (err != UNZ_OK)
      {
        writeLogLine("error " + toString(err) + " with zipfile in unzGoToNextFile", MODIO_DEBUGLEVEL_ERROR);
        unzClose(zipfile);
        return;
      }
    }
  }
  unzClose(zipfile);
  writeLogLine(zip_path + " extracted", MODIO_DEBUGLEVEL_LOG);
}

static int filetime(const char *filename, tm_zip *tmzip, uint32_t *dostime)
{
    int ret = 0;
#ifdef _WIN32
    FILETIME ftLocal;
    HANDLE hFind;
    WIN32_FIND_DATAA ff32;

    hFind = FindFirstFileA(filename, &ff32);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        FileTimeToLocalFileTime(&(ff32.ftLastWriteTime), &ftLocal);
        FileTimeToDosDateTime(&ftLocal,((LPWORD)dostime)+1,((LPWORD)dostime)+0);
        FindClose(hFind);
        ret = 1;
    }
#else
#if defined unix || defined __APPLE__
    struct stat s = {0};
    struct tm* filedate;
    time_t tm_t = 0;

    if (strcmp(filename,"-") != 0)
    {
        char name[MAXFILENAME+1];
        int len = strlen(filename);
        if (len > MAXFILENAME)
            len = MAXFILENAME;

        strncpy(name, filename, MAXFILENAME - 1);
        name[MAXFILENAME] = 0;

        if (name[len - 1] == '/')
            name[len - 1] = 0;

        /* not all systems allow stat'ing a file with / appended */
        if (stat(name,&s) == 0)
        {
            tm_t = s.st_mtime;
            ret = 1;
        }
    }

    filedate = localtime(&tm_t);

    tmzip->tm_sec  = filedate->tm_sec;
    tmzip->tm_min  = filedate->tm_min;
    tmzip->tm_hour = filedate->tm_hour;
    tmzip->tm_mday = filedate->tm_mday;
    tmzip->tm_mon  = filedate->tm_mon ;
    tmzip->tm_year = filedate->tm_year;
#endif
#endif
    return ret;
}

void compressFiles(std::string root_directory, std::vector<std::string> filenames, std::string zip_path)
{
  writeLogLine("Compressing " + modio::toString((u32)filenames.size()) + " files", MODIO_DEBUGLEVEL_LOG);

  writeLogLine(std::string("Compressing ") + " into " + zip_path, MODIO_DEBUGLEVEL_LOG);

  zipFile zf = NULL;
  //#ifdef USEWIN32IOAPI
  //  zlib_filefunc64_def ffunc = {0};
  //#endif
  const char *zipfilename = zip_path.c_str();
  const char *password = NULL;
  void *buf = NULL;
  size_t size_buf = WRITEBUFFERSIZE;
  int errclose = 0;
  int err = 0;
  int opt_overwrite = APPEND_STATUS_CREATE;
  int opt_compress_level = 9;
  int opt_exclude_path = 0;

  buf = malloc(size_buf);
  if (buf == NULL)
  {
    writeLogLine("Error allocating memory", MODIO_DEBUGLEVEL_ERROR);
  }

  //#ifdef USEWIN32IOAPI
  //  fill_win32_filefunc64A(&ffunc);
  //  zf = zipOpen2_64(zipfilename, opt_overwrite, NULL, &ffunc);
  //#else
  zf = zipOpen64(zipfilename, opt_overwrite);
  //#endif

  if (zf == NULL)
  {
    writeLogLine(std::string("Could not open ") + zipfilename, MODIO_DEBUGLEVEL_ERROR);
  }
  else
  {
    writeLogLine(std::string("Creating ") + zipfilename, MODIO_DEBUGLEVEL_LOG);
  }

  for (size_t i = 0; i < filenames.size(); i++)
  {
    if (filenames[i] == "modio.json")
      continue;
    std::string filename = filenames[i];
    std::string complete_file_path = root_directory + filename;
    FILE *fin = NULL;
    size_t size_read = 0;
    const char *filenameinzip = filename.c_str();
    const char *savefilenameinzip;
    zip_fileinfo zi = { };
    unsigned long crcFile = 0;
    int zip64 = 0;

    /* Get information about the file on disk so we can store it in zip */
    filetime(complete_file_path.c_str(), &zi.tmz_date, &zi.dosDate);
    zip64 = is_large_file(complete_file_path.c_str());

    /* Construct the filename that our file will be stored in the zip as.
          The path name saved, should not include a leading slash.
          If it did, windows/xp and dynazip couldn't read the zip file. */
    savefilenameinzip = filenameinzip;
    while (savefilenameinzip[0] == '\\' || savefilenameinzip[0] == '/')
      savefilenameinzip++;
    /* Should the file be stored with any path info at all? */
    if (opt_exclude_path)
    {
      const char *tmpptr = NULL;
      const char *lastslash = 0;

      for (tmpptr = savefilenameinzip; *tmpptr; tmpptr++)
      {
        if (*tmpptr == '\\' || *tmpptr == '/')
          lastslash = tmpptr;
      }

      if (lastslash != NULL)
        savefilenameinzip = lastslash + 1; /* base filename follows last slash. */
    }

    /* Add to zip file */
    err = zipOpenNewFileInZip3_64(zf, savefilenameinzip, &zi,
                                  NULL, 0, NULL, 0, NULL /* comment*/,
                                  (opt_compress_level != 0) ? Z_DEFLATED : 0,
                                  opt_compress_level, 0,
                                  /* -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, */
                                  -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
                                  password, crcFile, zip64);

    if (err != ZIP_OK)
      writeLogLine(std::string("Could not open ") + filenameinzip + " in zipfile, zlib error: " + toString(err), MODIO_DEBUGLEVEL_ERROR);
    else
    {
      fin = fopen(complete_file_path.c_str(), "rb");
      if (fin == NULL)
      {
        writeLogLine(std::string("Could not open ") + filenameinzip + " for reading", MODIO_DEBUGLEVEL_ERROR);
      }
    }

    if (err == ZIP_OK)
    {
      /* Read contents of file and write it to zip */
      do
      {
        size_read = fread(buf, 1, size_buf, fin);
        if ((size_read < size_buf) && (feof(fin) == 0))
        {
          writeLogLine(std::string("Error in reading ") + filenameinzip, MODIO_DEBUGLEVEL_ERROR);
        }

        if (size_read > 0)
        {
          err = zipWriteInFileInZip(zf, buf, (unsigned int)size_read);
          if (err < 0)
            writeLogLine(std::string("Error in writing ") + filenameinzip + " in zipfile, zlib error: " + toString(err), MODIO_DEBUGLEVEL_ERROR);
        }
      } while ((err == ZIP_OK) && (size_read > 0));
    }

    if (fin)
      fclose(fin);

    if (err < 0)
      err = ZIP_ERRNO;
    else
    {
      err = zipCloseFileInZip(zf);
      if (err != ZIP_OK)
        writeLogLine(std::string("Error in closing ") + filenameinzip + " in zipfile, zlib error: " + toString(err), MODIO_DEBUGLEVEL_ERROR);
    }
  }

  errclose = zipClose(zf, NULL);

  if (errclose != ZIP_OK)
    writeLogLine(std::string("Error in closing ") + zipfilename + ", zlib error: " + toString(errclose), MODIO_DEBUGLEVEL_ERROR);

  free(buf);
}

void compressDirectory(std::string directory, std::string zip_path)
{
  directory = modio::addSlashIfNeeded(directory);
  writeLogLine("Compressing directory " + directory, MODIO_DEBUGLEVEL_LOG);
  std::vector<std::string> filenames = getFilenames(directory);
  for(u32 i=0; i<filenames.size(); i++)
  {
    filenames[i] = filenames[i];
  }
  compressFiles(directory, filenames, zip_path);
}

int is_large_file(const char* filename)
{
#ifdef _WIN32
  HANDLE h = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (h == INVALID_HANDLE_VALUE)
    return 0;
  LARGE_INTEGER size = {};
  GetFileSizeEx(h, &size);
  CloseHandle(h);
  return (size.QuadPart >= 0xffffffff);
#else
	FILE *file = fopen(filename, "rb");
  if (!file)
    return 0;

  fseeko(file, 0, SEEK_END);
  off_t pos = _ftello(file);
  fclose(file);

  return (pos >= 0xffffffff);
#endif
}
}
}
