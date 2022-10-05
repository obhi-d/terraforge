
#pragma once
#include <RenderDevice.h>

namespace terra
{
  class ImguiBackend
  {
  public:

  private:
    GfxProgram::handle            shader;
    std::shared_ptr<RenderDevice> renderer;
  };
}