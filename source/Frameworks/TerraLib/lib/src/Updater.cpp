
#include "Updater.h"
#include "Terra.h"
#include "ShaderBuilder.h"
#include "CurveData.h"
#include "Node.h"
#include "Image.h"
#include "Pipeline.h"

namespace terra
{
namespace glsl
{
#include "glsl/buffer.glsl"
#include "glsl/curve430.glsl"
#include "glsl/image.glsl"
#include "glsl/texture.glsl"

uint32 subtypeSize(DataType type)
{
  switch (type)
  {
  case DataType::eFloat:
  case DataType::eInt:
    return 4;
  case DataType::eInt2:
  case DataType::eFloat2:
    return 8;
  case DataType::eBool:
    return 1;
  }
  return 0;
}

std::string_view bufferReadType(DataType type)
{
  switch (type)
  {
  case DataType::eInt:
    return "int";
  case DataType::eInt2:
    return "ivec2";
  case DataType::eFloat:
    return "float";
  case DataType::eFloat2:
    return "vec2";
  case DataType::eBool:
    return "bool";
  }
  return "invalid";
}

std::string_view bufferWriteType(DataType type)
{
  switch (type)
  {
  case DataType::eInt:
    return "ivec4";
  case DataType::eInt2:
    return "imat2x4";
  case DataType::eFloat:
    return "vec4";
  case DataType::eFloat2:
    return "mat2x4";
  case DataType::eBool:
    return "bvec4";
  }
  return "invalid";
}


void declPreamble(std::string& ubo, std::string& opaque, DescriptorList& dl, OptionList& ol, ParameterMeta& pm,
                      int32& offset, ShaderBuilder& sb)
{
  pm.optionIndex = (uint32_t)ol.size();
  ol.push_back("Has_" + pm.name);

  opaque += std::format("const bool has_{0} = bool(Has_{0});\n", pm.name);
}

void declBufferSource(std::string& ubo, std::string& opaque, DescriptorList& dl, OptionList& ol, ParameterMeta& pm,
                      int32& offset, ShaderBuilder& sb)
{
  
  sb.append(std::format("#define {0}_t {1}\n"
                        "#define {0}_t4 {2}\n",
                        pm.name, bufferReadType(pm.format.scalarSubType), bufferWriteType(pm.format.scalarSubType)));

  declPreamble(ubo, opaque, dl, ol, pm, offset, sb);

  auto bi = sb.declBuffer("U", pm.name, Access::eReadonly);
  pm.descriptorIndex = (int)dl.size();
  dl.emplace_back(bi.descriptor);
  opaque += bi.content;
  opaque += std::format("{{ {0}_t data[]; }}{0};\n", pm.name);
  opaque += std::format(gs_bufferLoad, pm.name);

  ubo += std::format("  {0}_t {0};", pm.name);
  pm.uboOffset = offset;
  offset += subtypeSize(pm.format.scalarSubType);
}
void declImageArray(std::string& ubo, std::string& opaque, DescriptorList& dl, OptionList& ol, ParameterMeta& pm,
  int32& offset, ShaderBuilder& sb)
{
  declPreamble(ubo, opaque, dl, ol, pm, offset, sb);

  auto bi = sb.declTextureArray(pm.name);
  pm.descriptorIndex = (int)dl.size();
  dl.emplace_back(bi.descriptor);
  opaque += bi.content;
  opaque += ";\n";
  opaque += std::format(gs_textureArrayLoad, pm.name);

  ubo += std::format("#if Has_{0}\n" 
                     "  vec2  uv_scale_{0};\n"
                     "  vec2  uv_off_{0};\n"
                     "#else\n" 
                     "  vec4 {0};\n"
                     "#endif\n" 
    , pm.name);
  pm.uboOffset = offset;
  offset += 16;
}
void declImageSource(std::string& ubo, std::string& opaque, DescriptorList& dl, OptionList& ol, ParameterMeta& pm,
                     int32& offset, ShaderBuilder& sb)
{
  declPreamble(ubo, opaque, dl, ol, pm, offset, sb);

  auto bi = sb.declTexture(pm.name);
  pm.descriptorIndex = (int)dl.size();
  dl.emplace_back(bi.descriptor);
  opaque += bi.content;
  opaque += ";\n";
  opaque += std::format(gs_textureLoad, pm.name);

   ubo += std::format("#if Has_{0}\n"
                     "  vec2  uv_scale_{0};\n"
                     "  vec2  uv_off_{0};\n"
                     "#else\n"
                     "  vec4 {0};\n"
                     "#endif\n",
                     pm.name);
  pm.uboOffset = offset;
  offset += 16;
}
void declCurveData(std::string& ubo, std::string& opaque, DescriptorList& dl, OptionList& ol, ParameterMeta& pm,
  int32& offset, ShaderBuilder& sb)
{
  declPreamble(ubo, opaque, dl, ol, pm, offset, sb);

  auto bi = sb.declBuffer("U", pm.name, Access::eReadonly);
  pm.descriptorIndex = (int)dl.size();
  dl.emplace_back(bi.descriptor);
  opaque += bi.content;
  opaque += std::format("{{ float data[]; }}{0};\n", pm.name);
  opaque += std::format(gs_curve430, pm.name);

  ubo += std::format("#if Has_{0}\n"
                     "  uint  np_{0};\n"
                     "  float c0_{0};\n"
                     "#else\n"
                     "  float  x_{0};\n"
                     "  float  s_{0};\n"
                     "#endif\n",
                     pm.name);
  pm.uboOffset = offset;
  offset += 8;
}

void declTextureOutput(std::string& opaque, DescriptorList& dl, OptionList& ol, int32_t& binding,
  ShaderBuilder& sb)
{
  sb.append("#define Has_TextureOutput 1\n");
  auto bi = sb.declImage("output_data", ImageFormat::eFloat, Access::eWriteonly);
  binding = (int)dl.size();
  dl.emplace_back(bi.descriptor);
  opaque += bi.content;
  opaque += ";\n";
  opaque += std::format(gs_imageStore, "output_data");
}

void declBufferOutput(std::string& ubo, std::string& opaque, DescriptorList& dl, OptionList& ol, int32_t& binding,
  ShaderBuilder& sb)
{
  sb.append("#define Has_TextureOutput 0\n");
  auto bi = sb.declBuffer("U", "Output", Access::eWriteonly);
  binding = (int)dl.size();
  dl.emplace_back(bi.descriptor);
  opaque += bi.content;
  opaque += "{ output_t data[]; }output_data;";
  opaque += std::format(gs_bufferStore, "output_data");
}

void fillScalarDisabled(ParameterMeta const& pm, ScalarValue sv, std::byte* data) 
{
  switch (pm.format.scalarSubType)
  {
  case DataType::eInt2:
  case DataType::eFloat2:
    std::memcpy(data, sv.value2.data(), sizeof(sv.value2));
    break;
  case DataType::eFloat:
  case DataType::eInt:
    std::memcpy(data, sv.value2.data(), sizeof(sv.value));
    break;
  }
}


} // namespace glsl
// ----------------------------------------------

// ------------------ Update --------------------
void CurveData::fillDescriptor(Pipeline const& pipeline, GfxDescriptorSet::rhandle& rh, std::byte* data)
{
  rh.first = handle;
  *(int*)data = spline.get_nb_points();
  *((float*)data + 1) = spline.get_c0();
}

void Node::fillDescriptor(Pipeline const& pipeline, GfxDescriptorSet::rhandle& rh, std::byte* data)
{
  if (isEnabled(pipeline))
  {
    if (hasTextureOutput())
    {
      rh.first           = pipeline.getOutputImage(self);
      *(vec2*)data       = vec2{1.f, 1.f};
      *(vec2*)(data + 8) = vec2{0.f, 0.f};
    }
    else
    {
      rh.first = pipeline.getOutputBuffer(self);
    }
  }
  else
    *(float*)data = defaultValue;
}

void Image::fillDescriptor(Pipeline const& pipeline, GfxDescriptorSet::rhandle& rh, std::byte* data) 
{
  rh.first           = handle;
  *(vec2*)data       = vec2{1.f, 1.f};
  *(vec2*)(data + 8) = vec2{0.f, 0.f};
}

void ImageSource::fillDescriptor(Pipeline const& pipeline, GfxDescriptorSet::rhandle& rh, std::byte* data)
{
  bool hasSource = true;
  if (DataSource::isValid(source) && isWithinTile(pipeline.params().tile))
  {
    DataSource& ds = get().get<DataSource>(source);
    if (ds.getType() == Type::eImage)
    {
      auto& image = (Image&)ds;
      rh.first = image.handle;
    }
    else if (ds.getType() == Type::eNode)
    {
      auto& node = (Node&)ds;
      rh.first   = pipeline.getOutputImage(node.getSelf());
    }
    *(vec2*)data       = uvScale;
    *(vec2*)(data + 8) = uvOffset;
  }
  else
  {
    *(float*)data       = defaultValue;
  }
}
// ----------------------------------------------
// ----------------- Options --------------------
bool CurveData::isEnabled(Pipeline const& pipeline)
{
  return true;
}

bool Image::isEnabled(Pipeline const& pipeline)
{
  return true;
}

bool ImageSource::isEnabled(Pipeline const& pipeline)
{
  DataSource& ds = get().get<DataSource>(source);
  if (DataSource::isValid(source) && isWithinTile(pipeline.params().tile))
    return true;
  return false;
}

bool Node::isEnabled(Pipeline const& pipe)
{
  if (meta->attribTileConstanted)
  {
    return isWithinTile(pipe.params().tile, tileConstraintMin, tileConstraintMax);
  }
  return true;
}

} // namespace terra