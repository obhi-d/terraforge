
#include "ResourceUtils.h"
#include "Logger.h"

namespace terra
{
Logger logger;

void   Logger::open(int level)
{
  out = std::move(std::ofstream(getMediaPath() / "terra.log"));
  this->level = level;
}

Logger& Logger::get() 
{
  return logger;
}
}