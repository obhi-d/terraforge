
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace terra
{

std::filesystem::path const&       getMediaPath();
std::vector<std::filesystem::path> getThemes();
std::vector<char>                  fileContentToBytes(std::string);
std::string                        fileContentToString(std::string);
} // namespace terra