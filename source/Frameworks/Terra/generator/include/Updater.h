
#pragma once
#include "NodeMeta.h"
#include <RenderResource.h>
#include <string>
#include <vector>

namespace terra
{
class Pipeline;
namespace glsl
{
using DescriptorList = std::vector<GfxDescriptorSetLayout::Descriptor>;
using OptionList     = std::vector<std::string>;

std::string_view bufferReadType(DataType type);
std::string_view bufferWriteType(DataType type);

void declBufferSource(std::string& ubo, std::string& opaque, DescriptorList&, OptionList&, ParameterMeta&,
                      int32_t& offset, ShaderBuilder&);
void declImageArray(std::string& ubo, std::string& opaque, DescriptorList&, OptionList&, ParameterMeta&,
                    int32_t& offset, ShaderBuilder&);
void declImageSource(std::string& ubo, std::string& opaque, DescriptorList&, OptionList&, ParameterMeta&,
                     int32_t& offset, ShaderBuilder&);
void declCurveData(std::string& ubo, std::string& opaque, DescriptorList&, OptionList&, ParameterMeta&, int32_t& offset,
                   ShaderBuilder&);
void declTextureOutput(std::string& opaque, DescriptorList&, OptionList&, int32_t& binding, ShaderBuilder&);
void declBufferOutput(std::string& opaque, DescriptorList& dl, OptionList& ol, int32_t& binding,
                      ShaderBuilder& sb);
void fillScalarDisabled(Pipeline const&, ParameterMeta const& pm, ScalarValue sv, ubyte_t* data);

} // namespace glsl

} // namespace terra