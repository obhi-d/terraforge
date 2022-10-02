
#pragma once
#include <cstdint>
#include <string>

namespace terra
{

struct ShaderBuilder
{
  struct BindingInfo
  {
    std::string content;
    int32_t     binding = 0;
  };

  virtual std::string_view preamble()                                                                = 0;
  virtual BindingInfo      declBuffer(std::string_view prefix, std::string_view name, bool readOnly) = 0;
  virtual BindingInfo      declConstants(std::string_view prefix, std::string_view name)             = 0;
  virtual BindingInfo      declTexture(std::string_view name)                                        = 0;
  virtual BindingInfo      declImage(std::string_view name)                                          = 0;
};

} // namespace terra