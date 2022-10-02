
#include "Dependency.h"
#include "Terra.h"

namespace terra
{

void Dependency::propagate(NodeEvent nev)
{
  for (auto i : dependencies)
  {
    Terra::get().propagate(i, nev);
  }
}
} // namespace terra