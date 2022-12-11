
#include "hyb/HybridNodeMeta.h"
#include "ShaderOptions.h"
#include "fmt/format.h"

namespace terra
{

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
}

} // namespace terra