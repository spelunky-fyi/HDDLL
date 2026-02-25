
#include "hddll/utils.h"

#include <algorithm>
#include <format>

#include "hddll/hd.h"
#include "hddll/hddll.h"

#include <imgui.h>

namespace hddll {

std::vector<std::string> split(const std::string &str, char delim) {
  std::vector<std::string> strings;
  size_t start;
  size_t end = 0;
  while ((start = str.find_first_not_of(delim, end)) != std::string::npos) {
    end = str.find(delim, start);
    strings.push_back(str.substr(start, end - start));
  }
  return strings;
}

std::string join(const std::vector<std::string> &strings, std::string delim) {
  std::string result;
  for (size_t i = 0; i < strings.size(); i++) {
    result += strings[i];
    if (i < strings.size() - 1) {
      result += delim;
    }
  }
  return result;
}

void ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
          }));
}

void rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       [](unsigned char ch) { return !std::isspace(ch); })
              .base(),
          s.end());
}

std::string formatLevel(uint8_t levelNumber) {

  auto world = 0;
  auto level = 0;

  if (levelNumber <= 4) {
    world = 1;
    level = levelNumber;
  } else if (levelNumber <= 8) {
    world = 2;
    level = levelNumber - 4;
  } else if (levelNumber <= 12) {
    world = 3;
    level = levelNumber - 8;
  } else if (levelNumber <= 16) {
    world = 4;
    level = levelNumber - 12;
  } else if (levelNumber <= 20) {
    world = 5;
    level = levelNumber - 16;
  }

  return std::format("{}-{}", world, level);
}

TextureDefinition *getTextureById(int32_t texture_id) {
  TextureDefinition *texture_def;

  for (int texture_idx = 0; texture_idx < 256; texture_idx++) {
    texture_def = &gGlobalState->_34struct->texture_defs[texture_idx];
    if (texture_def->texture_id == texture_id) {
      return texture_def;
    }
  }
  return NULL;
}

ImVec2 screenToGame(ImVec2 screen) {
  if (gDisplayWidth == 0)
    return {0, 0};

  auto x =
      (screen.x - ((float)gDisplayWidth / 2)) * (20 / (float)gDisplayWidth) +
      gCameraState->camera_x;
  auto y =
      (screen.y - ((float)gDisplayHeight / 2)) * -(20 / (float)gDisplayWidth) +
      gCameraState->camera_y;

  return {x, y};
}

ImVec2 gameToScreen(ImVec2 game) {
  if (gDisplayWidth == 0)
    return {0, 0};

  auto x = (game.x - gCameraState->camera_x) / (20 / (float)gDisplayWidth) +
           ((float)gDisplayWidth / 2);
  auto y = (game.y - gCameraState->camera_y) / -(20 / (float)gDisplayWidth) +
           ((float)gDisplayHeight / 2);
  return {x, y};
}

} // namespace hddll
