#include "c++/schemas/GameTagOption.h"

namespace modio
{
void GameTagOption::initialize(ModioGameTagOption modio_game_tag_option)
{
  hidden = modio_game_tag_option.hidden;

  if (modio_game_tag_option.name)
    name = modio_game_tag_option.name;
  if (modio_game_tag_option.type)
    type = modio_game_tag_option.type;

  tags.resize(modio_game_tag_option.tags_array_size);
  for (u32 i = 0; i < modio_game_tag_option.tags_array_size; i++)
  {
    tags[i] = modio_game_tag_option.tags_array[i];
  }
}

nlohmann::json toJson(GameTagOption &game_tag_option)
{
  nlohmann::json game_tag_option_json;

  game_tag_option_json["hidden"] = game_tag_option.hidden;
  game_tag_option_json["name"] = game_tag_option.name;
  game_tag_option_json["type"] = game_tag_option.type;

  nlohmann::json tags_json;
  for (auto &tag : game_tag_option.tags)
  {
    tags_json.push_back(tag);
  }
  game_tag_option_json["tags"] = tags_json;

  return game_tag_option_json;
}
} // namespace modio
