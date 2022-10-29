
#pragma once

#include "GlGfx.h"
#include "Pipeline.h"
#include "Setup.h"

namespace terra
{
class TerraMainApp;
class MeshPreview
{
public:
  void init(TerraMainApp&);
  void regenerate(TerraMainApp const&, dshandle);

private:
  std::shared_ptr<Pipeline> pipeline;
  GfxBuffer::handle         buffer;
  GfxMesh::handle           layout;
  GfxMaterial               material;
};
} // namespace terra