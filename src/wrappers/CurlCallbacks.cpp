#include "wrappers/CurlCallbacks.h"

namespace modio
{
namespace curlwrapper
{

void onJsonRequestFinished(CURL *curl)
{
  JsonResponseHandler *ongoing_call = g_ongoing_calls[curl];
  u32 response_code;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
  nlohmann::json response_json = modio::toJson(ongoing_call->response);

  if (ongoing_call->headers.find("X-Ratelimit-RetryAfter") != ongoing_call->headers.end())
  {
    u32 x_ratelimit_retryafter = stoul(ongoing_call->headers["X-Ratelimit-RetryAfter"]);
    modio::RETRY_AFTER = modio::getCurrentTime() + x_ratelimit_retryafter;
    modio::writeLogLine("API request limit hit. Could not poll events. Rerying after " + modio::toString(modio::RETRY_AFTER), MODIO_DEBUGLEVEL_WARNING);
  }

  if (ongoing_call->headers.find("X-RateLimit-Remaining") != ongoing_call->headers.end())
  {
    std::string x_rate_limit_remaining = ongoing_call->headers["X-RateLimit-Remaining"];
    if (x_rate_limit_remaining[x_rate_limit_remaining.size() - 1] == '\n')
      x_rate_limit_remaining.pop_back();
    writeLogLine("X-RateLimit-Remaining: " + x_rate_limit_remaining, MODIO_DEBUGLEVEL_LOG);
  }

  writeLogLine("Json request Finished. Response code: " + toString(response_code), MODIO_DEBUGLEVEL_LOG);
  if (response_code >= 400 && response_code <= 599)
  {
    writeLogLine(response_json.dump(), MODIO_DEBUGLEVEL_ERROR);
  }
  ongoing_call->callback(ongoing_call->call_number, response_code, response_json);
  g_ongoing_calls.erase(curl);
  delete ongoing_call;
  g_call_count++;
}

void onDownloadFinished(CURL *curl)
{
  OngoingDownload *ongoing_download = g_ongoing_downloads[curl];

  u32 response_code;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

  if (response_code >= 200 || response_code < 300)
  {
    writeLogLine("Download finished successfully.", MODIO_DEBUGLEVEL_LOG);
  }
  else
  {
    writeLogLine("Response code: " + modio::toString(response_code) + " Could not download form: " + ongoing_download->url, MODIO_DEBUGLEVEL_LOG);
  }

  ongoing_download->callback(ongoing_download->call_number, response_code);
  g_call_count++;
  g_ongoing_downloads.erase(curl);
  delete ongoing_download;
}

void onModDownloadFinished(CURL *curl)
{
  fclose(g_current_mod_download->file);
  g_current_mod_download->file = NULL;

  if (g_current_mod_download->queued_mod_download->state == MODIO_MOD_DOWNLOADING)
  {
    std::string installation_path = modio::getModIODirectory() + "mods/" + modio::toString(g_current_mod_download->queued_mod_download->mod_id) + "/";
    std::string downloaded_zip_path = g_current_mod_download->queued_mod_download->path;
    nlohmann::json mod_json = modio::toJson(g_current_mod_download->queued_mod_download->mod);

    addToDownloadedModsJson(installation_path, downloaded_zip_path, mod_json);
    
    writeLogLine("Finished downloading mod", MODIO_DEBUGLEVEL_LOG);

    u32 response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    if (response_code >= 200 && response_code < 300)
    {
      writeLogLine("Download finished successfully. Mod id: " + toString(g_current_mod_download->queued_mod_download->mod_id) + " Url: " + g_current_mod_download->queued_mod_download->url, MODIO_DEBUGLEVEL_LOG);
    }
    else
    {
      writeLogLine("Response code: " + modio::toString(response_code) + " Mod id: " + modio::toString(g_current_mod_download->queued_mod_download->mod_id), MODIO_DEBUGLEVEL_ERROR);
    }

    if (modio::download_callback)
    {
      modio::download_callback(response_code,  g_current_mod_download->queued_mod_download->mod.id);
    }

    g_mod_download_queue.remove(g_current_mod_download->queued_mod_download);
    delete g_current_mod_download->queued_mod_download;

    delete g_current_mod_download;
    g_current_mod_download = NULL;
    
    updateModDownloadQueueFile();
    downloadNextQueuedMod();
  }
  else if (g_current_mod_download->queued_mod_download->state == MODIO_MOD_PAUSING)
  {
    modio::writeLogLine("Mod " + modio::toString(g_current_mod_download->queued_mod_download->mod_id) + " download paused", MODIO_DEBUGLEVEL_LOG);
    g_current_mod_download->queued_mod_download->state = MODIO_MOD_PAUSED;
  }
  else if (g_current_mod_download->queued_mod_download->state == MODIO_PRIORITIZING_OTHER_DOWNLOAD)
  {
    modio::writeLogLine("Mod " + modio::toString(g_current_mod_download->queued_mod_download->mod_id) + " download paused. Another mod is being prioritized.", MODIO_DEBUGLEVEL_LOG);
    g_current_mod_download->queued_mod_download->state = MODIO_MOD_QUEUED;
    updateModDownloadQueue();
    downloadNextQueuedMod();
  }
  updateModDownloadQueueFile();
}

void onModfileUploadFinished(CURL *curl)
{
  writeLogLine("Upload Finished. Mod id: " + toString(g_current_modfile_upload->queued_modfile_upload->mod_id) /*+ " Url: " + current_queued_modfile_upload->url*/, MODIO_DEBUGLEVEL_LOG);

  if (g_current_modfile_upload->queued_modfile_upload->state == MODIO_MOD_UPLOADING)
  {
    u32 response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    if (modio::upload_callback)
    {
      modio::upload_callback(response_code, g_current_modfile_upload->queued_modfile_upload->mod_id);
    }

    delete g_current_modfile_upload->queued_modfile_upload;
    g_modfile_upload_queue.remove(g_current_modfile_upload->queued_modfile_upload);

    delete g_current_modfile_upload;
    g_current_modfile_upload = NULL;
    
    updateModUploadQueueFile();
    uploadNextQueuedModfile();
  }
  updateModUploadQueueFile();
}
} // namespace curlwrapper
} // namespace modio
