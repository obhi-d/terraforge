
#include "ResourceUtils.h"
#include <string_view>
#if defined(_WIN32)
#include <Windows.h>
#endif
#include <fstream>

namespace terra
{
std::filesystem::path const& getMediaPath()
{
  static std::filesystem::path stored =
    []()
  {
#if defined(_WIN32)
    {
      char16_t            result[512] = {};
      uint32_t            count       = GetModuleFileNameW(NULL, (LPWSTR)result, 512);
      std::u16string_view view{result, count};
      auto                last = view.find_last_of(u'\\');
      if (last != view.npos)
        view = view.substr(0, last);
      return std::filesystem::path(view);
    }
#elif defined(__unix__)
    {
      char8_t            result[1024] = {};
      uint32             count        = (uint32)readlink("/proc/self/exe", (char*)result, 1024);
      std::u8string_view view{result, count};
      auto               last = view.find_last_of(u8'/');
      if (last != view.npos)
        view = view.substr(0, last);
      return std::filesystem::path(view);
    }
#else
    return std::filesystem::current_path();
#endif
  }()
      .parent_path() /
    "media";
  return stored;
}

std::vector<std::filesystem::path> getThemes() 
{
  std::vector<std::filesystem::path> themes;
  // directory_iterator can be iterated using a range-for loop
  for (auto const& dir_entry : std::filesystem::directory_iterator{getMediaPath() / "themes"})
  {
    if(dir_entry.path().extension() == ".tns")
      themes.push_back(dir_entry.path());
  }
  return themes;
}

std::string fileContentToString(std::string name) 
{
  auto path = getMediaPath() / name;
  std::ifstream iff(path);
  if (iff.is_open())
  {
    return std::string((std::istreambuf_iterator<char>(iff)), std::istreambuf_iterator<char>());
  }
  return "";
}

std::vector<std::byte> fileContentToBytes(std::string name)
{
  auto          path = getMediaPath() / name;
  std::basic_ifstream<std::byte> iff(path, std::ios::binary | std::ios::in);
  if (iff.is_open())
  {
    return std::vector((std::istreambuf_iterator<std::byte>(iff)), std::istreambuf_iterator<std::byte>());
  }
  return {};
}

} // namespace terra