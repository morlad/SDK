#include "extern_c/schemas/ModioUser.h"

extern "C"
{
  void modioInitUser(ModioUser* user, json user_json)
  {
    user->id = -1;
    if(modio::hasKey(user_json, "id"))
    {
      user->id = user_json["id"];
    }

    user->date_online = -1;
    if(modio::hasKey(user_json, "date_online"))
    {
      user->date_online = user_json["date_online"];
    }

    user->username = NULL;
    if(modio::hasKey(user_json, "username"))
    {
      std::string username_str = user_json["username"];
      user->username = new char[username_str.size() + 1];
      strcpy_s(user->username, username_str.size() + 1, username_str.c_str());
    }

    user->name_id = NULL;
    if(modio::hasKey(user_json, "name_id"))
    {
      std::string name_id_str = user_json["name_id"];
      user->name_id = new char[name_id_str.size() + 1];
      strcpy_s(user->name_id, name_id_str.size() + 1, name_id_str.c_str());
    }

    user->timezone = NULL;
    if(modio::hasKey(user_json, "timezone"))
    {
      std::string timezone_str = user_json["timezone"];
      user->timezone = new char[timezone_str.size() + 1];
      strcpy_s(user->timezone, timezone_str.size() + 1, timezone_str.c_str());
    }

    user->language = NULL;
    if(modio::hasKey(user_json, "language"))
    {
      std::string language_str = user_json["language"];
      user->language = new char[language_str.size() + 1];
      strcpy_s(user->language, language_str.size() + 1, language_str.c_str());
    }

    user->profile_url = NULL;
    if(modio::hasKey(user_json, "profile_url"))
    {
      std::string profile_url_str = user_json["profile_url"];
      user->profile_url = new char[profile_url_str.size() + 1];
      strcpy_s(user->profile_url, profile_url_str.size() + 1, profile_url_str.c_str());
    }

    if(modio::hasKey(user_json, "avatar"))
    {
      modioInitAvatar(&(user->avatar), user_json["avatar"]);
    }
  }

  void modioFreeUser(ModioUser* user)
  {
    delete[] user->username;
    delete[] user->name_id;
    delete[] user->timezone;
    delete[] user->language;
  }
}
