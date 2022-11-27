
#include "ShaderOptions.h"

namespace terra
{
std::unordered_map<std::string_view, uint32_t> ShaderOptions::optionIndices;
std::vector<std::string_view>                  ShaderOptions::optionNames = {""};

} // namespace terra