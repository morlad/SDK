#include "CurlWrapper.h"

namespace modworks
{
  map<CURL*, JsonResponseHandler*> ongoing_calls;

  CurrentDownloadHandle* current_download_handle;

  CurrentDownloadInfo current_download_info;

  int call_count = 0;
  int ongoing_call = 0;

  void initCurl()
  {
    current_download_handle = new CurrentDownloadHandle;
    current_download_handle->path = "";
    current_download_handle->pause_flag = false;
    current_download_handle->curl = NULL;

    current_download_info.url = "";
    current_download_info.download_total = 0;
    current_download_info.download_progress = 0;

    if(curl_global_init(CURL_GLOBAL_ALL))
      writeLogLine("Curl initialized", verbose);
    else
      writeLogLine("Error initializing curl", error);
  }

  int getCallCount()
  {
    return call_count;
  }

  int getOngoingCall()
  {
    return ongoing_call;
  }

  void advanceCallCount()
  {
    call_count++;
  }

  void advanceOngoingCall()
  {
    ongoing_call++;
  }

  void lockCall(int call_number)
  {
    while(call_number!=getOngoingCall());
  }

  JsonResponseHandler::JsonResponseHandler()
  {
    this->response = "";
  }

  struct data
  {
    char trace_ascii; /* 1 or 0 */
  };

  int json_response_trace(CURL *handle, curl_infotype type,
               char *data, size_t size,
               void *userp)
  {
    (void)handle; /* prevent compiler warning */
    if(type == CURLINFO_DATA_IN)
    {
      ongoing_calls[handle]->response.append(data, size);
    }

    return 0;
  }

  void get(int call_number, string url, vector<string> headers, function<void(int call_number, int response_code, json response)> callback)
  {
    writeLogLine("getJsonCall call to " + url, verbose);
    lockCall(call_number);
    CURL *curl;
    CURLcode res;
    long response_code = 0;

    struct data config;
    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();

    ongoing_calls[curl] = new JsonResponseHandler();
    if(curl) {
      struct curl_slist *chunk = NULL;
      for(int i=0;i<(int)headers.size();i++)
        chunk = curl_slist_append(chunk, headers[i].c_str());

      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

      curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, json_response_trace);
      curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &config);
      curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
      /* Perform the request, res will get the return code */
      res = curl_easy_perform(curl);
      /* Check for errors */
      if(res == CURLE_OK)
      {
        curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &response_code);
      }
      else
      {
        writeLogLine(string("curl_easy_perform() failed: ") + curl_easy_strerror(res), error);
        response_code = 0;
      }

      curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &response_code);
      /* always cleanup */
      curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    ongoing_calls[curl]->response = dataToJsonString(ongoing_calls[curl]->response);

    json json_response;
    try
    {
      json_response = json::parse(ongoing_calls[curl]->response);
    }
    catch (json::parse_error &e)
    {
      writeLogLine(string("Error parsing json: ") + e.what(), error);
      response_code = 0;
      json_response = "{}"_json;
    }

    callback(call_number, response_code, json_response);
    advanceOngoingCall();
    writeLogLine("getJsonCall call to " + url + "finished", verbose);
  }

  size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
  {
    size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
    return written;
  }

  int progress_callback(void *clientp,   double dltotal,   double dlnow,   double ultotal,   double ulnow)
  {
    current_download_info.download_total = dltotal;
    current_download_info.download_progress = dlnow;

    if(current_download_handle->pause_flag)
    {
      curl_easy_pause(current_download_handle->curl , CURLPAUSE_RECV);

      string file_path = getModworksDirectory() + "paused_download.json";

      json paused_download_json;

      paused_download_json["path"] = current_download_handle->path;

      time_t  timev;
      time(&timev);
      paused_download_json["time"] = timev;

      paused_download_json["download_total"] = dltotal;
      paused_download_json["download_progress"] = dlnow;

      std::ofstream o(file_path);
      o << std::setw(4) << paused_download_json << std::endl;
      return -1;
    }

    return 0;
  }

  void shutdownCurl()
  {
    string path = current_download_handle->path;
    string extension = path.substr(path.length() - 4);

    for(int i=1; i<(int)extension.size();i++)
      extension[i] = tolower(extension[i]);

    if(extension == ".zip")
    {
      current_download_handle->pause_flag = true;
    }

    ongoing_call = -1;
    call_count = -1;
    ongoing_calls.clear();
  }

  curl_off_t getProgressIfStored(string path)
  {
    string file_path = getModworksDirectory() + "paused_download.json";
    std::ifstream in(file_path);
    json modfile_downloads_json;
    if(in.is_open())
    {
      in>>modfile_downloads_json;
      string path_stored = modfile_downloads_json["path"];
      curl_off_t download_progress_stored = modfile_downloads_json["download_progress"];
      if(path_stored == path)
      {
        return download_progress_stored;
      }
    }
    return 0;
  }

  CurrentDownloadInfo getCurrentDownloadInfo()
  {
    return current_download_info;
  }

  void download(int call_number, string url, string path, function< void(int call_number, int response_code, string url, string path) > callback)
  {
    writeLogLine("downloadFile call to " + url, verbose);
    lockCall(call_number);
    CURL *curl;
    CURLcode res;
    FILE *file;
    long response_code = 0;
    curl = curl_easy_init();

    current_download_handle->path = path;
    current_download_handle->pause_flag = false;
    current_download_handle->curl = curl;

    current_download_info.url = url;

    curl_off_t progress = getProgressIfStored(path);

    if(curl)
    {
      file = fopen(path.c_str(),"wb");
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

      if(progress != 0)
      {
        curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, progress);
      }

      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

      curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

      curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_callback);
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

      res = curl_easy_perform(curl);

      if(res == CURLE_OK)
      {
        curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &response_code);
      }
      else
      {
        writeLogLine(string("curl_easy_perform() failed: ") + curl_easy_strerror(res), error);
        response_code = 0;
      }

      curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &response_code);

      curl_easy_cleanup(curl);

      fclose(file);
    }

    if(current_download_handle)
    {
      current_download_handle->path = "";
      current_download_handle->pause_flag = false;
      current_download_handle->curl = NULL;
    }

    callback(call_number, response_code, url, path);
    advanceOngoingCall();
    writeLogLine("getJsonCall call to " + url + " finished", verbose);
  }

  void postForm(int call_number, string url, vector<string> headers, map<string, string> curlform_copycontents, map<string, string> curlform_files, function<void(int call_number, int response_code, json response)> callback)
  {
    writeLogLine(string("postForm call to ") + url, verbose);
    lockCall(call_number);
    CURL *curl;
    CURLcode res;
    long response_code = 0;

    struct curl_httppost *formpost=NULL;
    struct curl_httppost *lastptr=NULL;
    struct curl_slist *headerlist=NULL;
    static const char buf[] = "Expect:";

    curl_global_init(CURL_GLOBAL_ALL);

    for(map<string,string>::iterator i = curlform_files.begin();
          i!=curlform_files.end();
          i++)
    {
      curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, (*i).first.c_str(),
        CURLFORM_FILE, (*i).second.c_str(), CURLFORM_END);
    }

    for(map<string,string>::iterator i = curlform_copycontents.begin();
          i!=curlform_copycontents.end();
          i++)
    {
      curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, (*i).first.c_str(),
        CURLFORM_COPYCONTENTS, (*i).second.c_str(), CURLFORM_END);
    }

    /* Fill in the submit field too, even if this is rarely needed */
    curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME, "submit",
                 CURLFORM_COPYCONTENTS, "send",
                 CURLFORM_END);

    curl = curl_easy_init();

    ongoing_calls[curl] = new JsonResponseHandler();

    struct curl_slist *chunk = NULL;
    for(int i=0;i<(int)headers.size();i++)
      chunk = curl_slist_append(chunk, headers[i].c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
    /* initialize custom header list (stating that Expect: 100-continue is not
       wanted */
    headerlist = curl_slist_append(headerlist, buf);
    if(curl)
    {
      /* what URL that receives this POST */
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

      curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, json_response_trace);

      curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
      //if((argc == 2) && (!strcmp(argv[1], "noexpectheader")))
        /* only disable 100-continue header if explicitly requested */
        //curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
      curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
      res = curl_easy_perform(curl);

      if(res == CURLE_OK)
      {
        curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &response_code);
      }
      else
      {
        writeLogLine(string("curl_easy_perform() failed: ") + curl_easy_strerror(res), error);
        response_code = 0;
      }

      curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &response_code);

      curl_easy_cleanup(curl);
      curl_formfree(formpost);
      curl_slist_free_all(headerlist);
    }

    ongoing_calls[curl]->response = dataToJsonString(ongoing_calls[curl]->response);

    json json_response;
    try
    {
      json_response = json::parse(ongoing_calls[curl]->response);
    }
    catch (json::parse_error &e)
    {
      writeLogLine(string("Error parsing json: ") + e.what(), error);
      response_code = 0;
      json_response = "{}"_json;
    }

    callback(call_number, response_code, json_response);
    advanceOngoingCall();
    writeLogLine(string("postForm call to ") + url + " finished", verbose);
  }

  void post(int call_number, string url, map<string, string> data, function<void(int call_number, int response_code, json response)> callback)
  {
    writeLogLine(string("post call to ") + url, verbose);
    lockCall(call_number);

    CURL *curl;
    CURLcode res;
    long response_code = 0;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    ongoing_calls[curl] = new JsonResponseHandler();

    if(curl)
    {
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

      string str_data = "";
      for(map<string, string>::iterator i = data.begin(); i!=data.end(); i++)
      {
        if(i!=data.begin())
          str_data += "&";
        str_data += (*i).first + "=" + (*i).second;
      }

      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, str_data.c_str());
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

      curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, json_response_trace);

      curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

      res = curl_easy_perform(curl);

      if(res == CURLE_OK)
      {
        curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &response_code);
      }
      else
      {
        writeLogLine(string("curl_easy_perform() failed: ") + curl_easy_strerror(res), error);
        response_code = 0;
      }

      curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    ongoing_calls[curl]->response = dataToJsonString(ongoing_calls[curl]->response);

    json json_response;
    try
    {
      json_response = json::parse(ongoing_calls[curl]->response);
    }
    catch (json::parse_error &e)
    {
      writeLogLine(string("Error parsing json: ") + e.what(), error);
      response_code = 0;
      json_response = "{}"_json;
    }

    callback(call_number, response_code, json_response);
    advanceOngoingCall();
    writeLogLine(string("post call to ") + url + " finished", verbose);
  }
}
