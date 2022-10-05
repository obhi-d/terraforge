#pragma once

#include "GfxDeviceObjects.h"
#include "ShaderBuilder.h"
#include <sstream>

namespace terra
{
enum class BindingType
{
  eImage,
  eTexture,
  eConstants,
  eBuffer,
  kCount
};

class GfxShaderBuilder : public ShaderBuilder
{
public:
  GfxShaderBuilder(GfxFeature const& options);
  BindingInfo declBuffer(std::string_view prefix, std::string_view name, Access access) final;
  BindingInfo declConstants(std::string_view prefix, std::string_view name) final;
  BindingInfo declTexture(std::string_view name) final;
  BindingInfo declImage(std::string_view name, ImageFormat format, Access access) final;

  void begin(ShaderType) final;
  void end() final;
  void beginSection(Section) final;
  void endSection() final;

  void append(std::string_view value) final
  {
    section += value;
  }

  std::string_view content(ShaderType shader) const
  {
    return output[(uint32_t)shader];
  }

private:
  friend class GfxDevice;
  
  uint32_t                 imageBindingCounter    = 0;
  uint32_t                 textureBindingCounter  = 0;
  uint32_t                 constantBindingCounter = 0;
  uint32_t                 bufferBindingCounter   = 0;
  Section                  currentSection;
  std::string              section;
  std::string              declarations;
  std::string              output[ShaderTypeCount];
  GfxFeature const&        opt;
  ShaderType          type;
};
} // namespace terra