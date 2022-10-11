
#include "Dependency.h"
#include "Terra.h"

namespace terra
{

void Dependency::propagate(hnode src, NodeEvent nev)
{
  for (auto i : dependents)
  {
    Terra::get().propagate(src, i, nev);
  }
}
} // namespace terra