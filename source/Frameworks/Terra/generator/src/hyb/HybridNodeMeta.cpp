
#include "hyb/HybridNodeMeta.h"
#include "ResourceUtils.h"
#include "ShaderOptions.h"
#include "Terra.h"
#include "fmt/format.h"
#include "hyb/HybridNode.h"

namespace terra
{

GpuNodeMeta::GpuPipelineMap GpuNodeMeta::shaderMaps;

void GpuNodeMeta::prepare()
{
  ShaderOptions::Dictionary dict;
  std::string               imgPrefix  = "HasImage_";
  std::string               buffPrefix = "HasBuffer_";
  std::string               boolPrefix = "HasOption_";
  std::string               enumPrefix = "Enum_";

  for (uint32_t i = 0; i < parameterDef.size(); ++i)
  {
    auto const& pdef = parameterDef[i];
    switch (pdef.format.type)
    {
    case DataTypeEnum::eImage:
      dict.names.emplace_back(imgPrefix + std::string(pdef.name()));
      break;
    case DataTypeEnum::eBuffer:
      dict.names.emplace_back(buffPrefix + std::string(pdef.name()));
      break;
    case DataTypeEnum::eBool:
      dict.names.emplace_back(boolPrefix + std::string(pdef.name()));
      break;
    case DataTypeEnum::eEnum:
    {
      for (uint32_t i = 0; i < pdef.maxEnum; ++i)
        dict.names.emplace_back(fmt::format("Enum_{}", pdef.enumDisplayInfo[i].id));
    }
    break;
    }
  }

  dictionaryIdx = ShaderOptions::addDictionary(std::move(dict));
  HybridNodeMeta::prepare();
}

void GpuNodeMeta::registerKnownMeta()
{
  auto inf = std::numeric_limits<float>::infinity();
  {
    GpuNodeMeta meta;

    meta.displayInfo.from("ImageMask");
    meta.as<GpuImageNode>();

    meta.parameterDef.emplace_back(MemberPtr<&GpuImageNode::image>(), "source", ValueRange(), DataType::eImage,
                                   DataType::eFloat, ImageFormat::eFloat, ParamDeclType::eSampler2D,
                                   SemanticEnum::eNone, SamplerParam::eLinearWrap);
    meta.parameterDef.emplace_back(MemberPtr<&GpuImageNode::sampleScale>(), "sample_scale",
                                   ValueRange(0.0f, -inf, inf, 0.1f), DataType::eFloat2, DataType::eFloat2,
                                   ImageFormat::eFloat, ParamDeclType::eScalar);
    meta.parameterDef.emplace_back(MemberPtr<&GpuImageNode::sampleOffset>(), "sample_offset",
                                   ValueRange(0.0f, -inf, inf, 0.1f), DataType::eFloat2, DataType::eFloat2,
                                   ImageFormat::eFloat, ParamDeclType::eScalar);
    meta.parameterDef.emplace_back(MemberPtr<&GpuImageNode::scale>(), "scale", ValueRange(0.0f, -inf, inf, 0.1f),
                                   DataType::eFloat, DataType::eFloat, ImageFormat::eFloat, ParamDeclType::eScalar);

    meta.outputs.emplace_back("heights",
                              DataFormat(DataTypeEnum::eBuffer, DataTypeEnum::eFloat, ImageFormatEnum::eFloat,
                                         ParamDeclTypeEnum::eSampler2D, SemanticEnum::eHeights));
    meta.outputs.back().clear = true;
    meta.passes.emplace_back();
    GpuPass& pass      = meta.passes.back();
    pass.function      = "node";
    pass.shaderContent = fileContentToString("shaders/image_node.glsl");
    pass.outputs.emplace_back(0);
    pass.parameters            = {0, 1, 2, 3};
    pass.state.nbBlendModes    = 1;
    pass.state.blend[0].mode   = BlendMode::eDisabled;
    pass.state.cullMode        = CullMode::eCullBack;
    pass.state.depthTest       = DepthTestMode::eDisabled;
    pass.state.depthWrite      = false;
    pass.state.flush           = false;
    pass.state.scissorsEnabled = false;
    terra::get().addMeta(meta);
  }

  {
    GpuNodeMeta meta;

    meta.displayInfo.from("CurveMask");
    meta.as<GpuCurveNode>();
    meta.parameterDef.emplace_back(MemberPtr<&GpuCurveNode::curve>(), "source", ValueRange(), DataType::eCurveData,
                                   DataType::eFloat, ImageFormat::eNone, ParamDeclType::eReadonlySSBO);
    meta.parameterDef.emplace_back(MemberPtr<&GpuCurveNode::scale>(), "scale", ValueRange(0.0f, -inf, inf, 0.1f),
                                   DataType::eFloat2, DataType::eFloat2, ImageFormat::eFloat, ParamDeclType::eScalar);
    meta.outputs.emplace_back("heights",
                              DataFormat(DataTypeEnum::eBuffer, DataTypeEnum::eFloat, ImageFormatEnum::eFloat,
                                         ParamDeclTypeEnum::eSampler2D, SemanticEnum::eHeights));
    meta.outputs.back().clear = true;
    meta.passes.emplace_back();
    GpuPass& pass      = meta.passes.back();
    pass.function      = "node";
    pass.shaderContent = fileContentToString("shaders/curve_node.glsl");
    pass.outputs.emplace_back(0);
    pass.parameters            = {0, 1};
    pass.state.nbBlendModes    = 1;
    pass.state.blend[0].mode   = BlendMode::eDisabled;
    pass.state.cullMode        = CullMode::eCullBack;
    pass.state.depthTest       = DepthTestMode::eDisabled;
    pass.state.depthWrite      = false;
    pass.state.flush           = false;
    pass.state.scissorsEnabled = false;
    terra::get().addMeta(meta);
  }
}

GpuPipelinePtr GpuNodeMeta::findProgram(ProgramKey const& key) const
{
  auto it = shaderMaps.find(key);
  if (it != shaderMaps.end())
  {
    return it->second.lock();
  }
  return GpuPipelinePtr{};
}

void GpuNodeMeta::addProgram(ProgramKey const& key, GpuPipelinePtr program) const
{
  shaderMaps[key] = program;
}

} // namespace terra