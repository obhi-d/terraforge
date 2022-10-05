
#include "Logger.h"
namespace terra
{
Logger logger;

void   Logger::open(int level)
{
  out.open("terra.log");
  this->level = level;
}

Logger& Logger::get() 
{
  return logger;
}
}