
#pragma once
#include <cstdint>
#include <span>
#include <string>

namespace terra
{

using Options = uint64_t;
struct ShaderOptions
{
  std::span<std::string const> names;
  Options                      bitMask = {};
};

struct ShaderBuilder
{
  enum Section
  {
    eDecl,
    eMain
  };

  struct BindingInfo
  {
    std::string   content;
    GfxDescriptor descriptor;
  };

  virtual BindingInfo declBuffer(std::string_view prefix, std::string_view name, Access access) = 0;
  virtual BindingInfo declConstants(std::string_view prefix, std::string_view name)             = 0;
  virtual BindingInfo declTexture(std::string_view name)                                        = 0;
  virtual BindingInfo declTextureArray(std::string_view name)                                   = 0;
  virtual BindingInfo declImage(std::string_view name, ImageFormat format, Access access)       = 0;
  virtual void        append(std::string_view)                                                  = 0;
  virtual void        begin(ShaderType)                                                         = 0;
  virtual void        end()                                                                     = 0;
  virtual void        beginSection(Section)                                                     = 0;
  virtual void        endSection()                                                              = 0;
};

} // namespace terra