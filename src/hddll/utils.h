#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct ImVec2;

namespace hddll {

class TextureDefinition;

std::vector<std::string> split(const std::string &str, char delim);
std::string join(const std::vector<std::string> &strings, std::string delim);
void ltrim(std::string &s);
void rtrim(std::string &s);
std::string formatLevel(uint8_t levelNumber);
TextureDefinition *getTextureById(int32_t texture_id);

// Coordinate conversion
ImVec2 screenToGame(ImVec2 screen);
ImVec2 gameToScreen(ImVec2 game);

} // namespace hddll
