
#include "GfxShaderBuilder.h"
#include "GlGfxUtils.h"
#include "fmt/format.h"

namespace terra
{

GfxShaderBuilder::GfxShaderBuilder(GfxFeature const& options) : opt(options)
{
  this->type = type;
}

ShaderBuilder::BindingInfo GfxShaderBuilder::declBuffer(std::string_view prefix, std::string_view name, Access access)
{
  BindingInfo info;
  info.content =
    fmt::format("layout(std430, binding = {}) {} buffer {}{}", bufferBindingCounter, toString(access), prefix, name);
  info.descriptor.access  = access;
  info.descriptor.binding = bufferBindingCounter++;
  info.descriptor.type    = GfxDescriptorType::eBuffer;
  return info;
}
ShaderBuilder::BindingInfo GfxShaderBuilder::declConstants(std::string_view prefix, std::string_view name)
{
  BindingInfo info;
  info.content = fmt::format("layout(std140, binding = {}) uniform {}{}", constantBindingCounter, prefix, name);
  info.descriptor.access  = Access::eReadonly;
  info.descriptor.binding = constantBindingCounter++;
  info.descriptor.type    = GfxDescriptorType::eConstants;
  return info;
}
ShaderBuilder::BindingInfo GfxShaderBuilder::declTexture(std::string_view name)
{
  BindingInfo info;
  info.content            = fmt::format("layout(binding = {}) uniform sampler2D {}", textureBindingCounter, name);
  info.descriptor.access  = Access::eReadonly;
  info.descriptor.binding = textureBindingCounter++;
  info.descriptor.type    = GfxDescriptorType::eTexture;
  return info;
}
ShaderBuilder::BindingInfo GfxShaderBuilder::declTextureArray(std::string_view name)
{
  BindingInfo info;
  info.content            = fmt::format("layout(binding = {}) uniform sampler2DArray {}", textureBindingCounter, name);
  info.descriptor.access  = Access::eReadonly;
  info.descriptor.binding = textureBindingCounter++;
  info.descriptor.type    = GfxDescriptorType::eTexture;
  return info;
}
ShaderBuilder::BindingInfo GfxShaderBuilder::declImage(std::string_view name, ImageFormatEnum format, Access access)
{
  BindingInfo      info;
  std::string_view sformat;
  switch (format)
  {
  case ImageFormatEnum::eSrgb8Alpha8:
  case ImageFormatEnum::eRgba8:
    sformat = "rgba8";
    break;
  default:
    sformat = "r32f";
    break;
  }
  info.content = fmt::format("layout(binding = {}, {}) restrict {} uniform image2D {}", imageBindingCounter, sformat,
                             toString(access), name);
  info.descriptor.access  = access;
  info.descriptor.binding = textureBindingCounter++;
  info.descriptor.type    = GfxDescriptorType::eImage;
  return info;
}
void GfxShaderBuilder::begin(ShaderType t)
{
  type = t;
}
void GfxShaderBuilder::end()
{
  if (!section.empty())
    output[(uint32_t)type] += section;
  section.clear();
}

void GfxShaderBuilder::beginSection(Section s)
{
  currentSection = s;
}
void GfxShaderBuilder::endSection()
{
  if (currentSection == Section::eDecl)
    declarations += section;
  else
    output[(uint32_t)type] += section;
  section.clear();
}

} // namespace terra
