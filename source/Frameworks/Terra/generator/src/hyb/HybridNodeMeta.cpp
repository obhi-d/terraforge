
#include "hyb/HybridNodeMeta.h"
#include "ShaderOptions.h"

namespace terra
{

void GpuNodeMeta::prepare()
{
  ShaderOptions::Dictionary dict;
  std::string               imgPrefix  = "HasImage_";
  std::string               buffPrefix = "HasBuffer_";
  std::string               boolPrefix = "HasOption_";
  std::string               enumPrefix = "ActiveEnum_";

  for (uint32_t i = 0; i < parameterDef.size(); ++i)
  {
    auto const& pdef = parameterDef[i];
    switch (pdef.format.type)
    {
    case DataTypeEnum::eImage:
      dict.names.emplace_back(imgPrefix + std::string(pdef.name.substr(1)));
      break;
    case DataTypeEnum::eBuffer:
      dict.names.emplace_back(buffPrefix + std::string(pdef.name.substr(1)));
      break;
    case DataTypeEnum::eBool:
      dict.names.emplace_back(boolPrefix + std::string(pdef.name.substr(1)));
      break;
    case DataTypeEnum::eEnum:
      for (uint32_t i = 0; i < pdef.maxEnum; ++i)
        dict.names.emplace_back(enumPrefix + std::string(pdef.enumNames[i]));
      break;
    }
  }

  dictionaryIdx = (uint32_t)ShaderOptions::optionDictionaries.size();
  ShaderOptions::optionDictionaries.emplace_back(std::move(dict));
}

} // namespace terra