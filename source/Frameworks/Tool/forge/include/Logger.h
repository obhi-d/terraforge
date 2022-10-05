
#pragma once

#include <string>
#include <string_view>
#include <fstream>
#include <iterator>
#include <format>

namespace terra
{
class Logger
{
public:
  static inline constexpr int Debug = 3;
  static inline constexpr int Info  = 2;
  static inline constexpr int Warn  = 1;
  static inline constexpr int Error = 0;

  void           open(int level);
  static Logger& get();

public:
  template <typename ...Args>
  friend constexpr void log(int, std::string_view fmt, Args&&...);
  std::fstream out;
  int level = 0;
};

template <typename... Args>
constexpr void log(int level, std::string_view fmt, Args&&...args)
{
  auto& l = Logger::get();
  if (level <= l.level)
  {
    static constexpr std::string_view levels[] = {"ERROR", "WARN ", "INFO ", "DEBUG"}; 
    l.out << "\n[" << levels[level] << "] ";
    std::vformat_to(std::ostream_iterator<char>(l.out), fmt, std::make_format_args(args...));
  }
}

template <typename... Args>
constexpr void logDebug(Args&&... args)
{
  log(Logger::Debug, args...);
}

template <typename... Args>
constexpr void logInfo(Args&&... args)
{
  log(Logger::Info, args...);
}

template <typename... Args>
constexpr void logWarn(Args&&... args)
{
  log(Logger::Warn, args...);
}

template <typename... Args>
constexpr void logError(Args&&... args)
{
  log(Logger::Error, args...);
}

}