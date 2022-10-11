
#pragma once

#include "Setup.h"
#include "Pipeline.h"

namespace terra
{
  class MeshPreview
  {
  public:
    void regenerate(AppSettings const&, hnode);

  private:
    Pipeline          pipeline;
    GfxBuffer::handle buffer;
  };
}