
#include "ShaderOptions.h"

namespace terra
{

class SourceBuilder
{
  bool isMergable(float cycles);

  ShaderOptions options;
  float         cycles = 1.f;
};

} // namespace terra