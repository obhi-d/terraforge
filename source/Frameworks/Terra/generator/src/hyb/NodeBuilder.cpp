
#include "hyb/NodeBuilder.h"
#include "Node.h"
#include "ResourceUtils.h"
#include "Terra.h"
#include "hyb/HybridNode.h"
#include <fmt/format.h>
#include <format>

using namespace terra;
std::u8string_view terra::NodeCmdHandler::localizedString(std::string_view name)
{
  return terra::get().localizationProvider(name);
}

NodeTextHandler(textreg, build, state, type, name, content) {}

NodeCmdHandler(name, builder, state, cmd)
{
  builder.meta.displayInfo.from(terra::getIdxParam(cmd, 0));
  return neo::retcode::e_success;
}

NodeCmdHandler(icon, builder, state, cmd)
{
  auto p = terra::getIdxParam(cmd, 0);
  std::from_chars(p.data(), p.data() + p.size(), builder.meta.icon);
  return neo::retcode::e_success;
}

NodeCmdHandler(category, builder, state, cmd)
{
  builder.meta.category = builder.localizedString(terra::getIdxParam(cmd, 0));
  return neo::retcode::e_success;
}

NodeCmdHandler(style, builder, state, cmd)
{
  auto p = terra::getIdxParam(cmd, 0);
  std::from_chars(p.data(), p.data() + p.size(), builder.meta.style);
  return neo::retcode::e_success;
}

NodeCmdHandler(function, builder, state, cmd)
{
  builder.content.function = terra::getIdxParam(cmd, 0);
  return neo::retcode::e_success;
}

NodeCmdHandler(cache, builder, state, cmd)
{
  builder.meta.cacheResults = terra::getIdxParam(cmd, 0) == "true";
  return neo::retcode::e_success;
}

NodeCmdHandler(inject, builder, state, cmd)
{
  builder.meta.isSourceModifier = terra::getIdxParam(cmd, 0) == "true";
  return neo::retcode::e_success;
}
/*
NodeCmdHandler(iteration, builder, state, cmd)
{
  auto value = terra::getIdxParam(cmd, 0);
  if (value == "inf")
    builder.meta.iteration = std::numeric_limits<uint32_t>::max();
  else
    std::from_chars(value.data(), value.data() + value.length(), builder.meta.iteration);
  return neo::retcode::e_success;
}
*/

NodeCmdHandler(param, builder, state, cmd)
{
  terra::ParameterMeta meta;

  meta.name = std::string{terra::getIdxParam(cmd, 0)};
  meta.displayInfo.from(meta.name);
  auto const& params = cmd.params().value();
  for (auto& p : params)
  {
    if (p.index() != 1)
      continue;
    if (std::holds_alternative<neo::single>(p))
    {
      auto const& entry = std::get<neo::single>(p);
      if (entry.name() == "type")
      {
        meta.setTypeFromString(entry.value(), "float");
      }
      else if (entry.name() == "min")
      {
        meta.setValueFromString(terra::ParameterMeta::ValueType::eMin, entry.value());
      }
      else if (entry.name() == "max")
      {
        meta.setValueFromString(terra::ParameterMeta::ValueType::eMax, entry.value());
      }
      else if (entry.name() == "default")
      {
        meta.setValueFromString(terra::ParameterMeta::ValueType::eDefault, entry.value());
      }
      else if (entry.name() == "step")
      {
        meta.setValueFromString(terra::ParameterMeta::ValueType::eStep, entry.value());
      }
      else if (entry.name() == "decltype")
      {
        meta.setDeclFromString(entry.value());
      }
      else if (entry.name() == "preeval")
      {
        meta.format.preEval = true;
      }
      else if (entry.name() == "format")
      {
        meta.format.preEval = true;
      }
    }
    else if (std::holds_alternative<neo::list>(p))
    {
      auto const& entry = std::get<neo::list>(p);
      if (entry.name() == "type")
      {
        auto const& values = entry.value();
        meta.setTypeFromString(std::get<neo::single>(values[0]).value(), std::get<neo::single>(values[1]).value());
      }
      else if (entry.name() == "enums")
      {
        auto& values  = entry.value();
        auto  maxEnum = values.size();
        meta.maxEnum  = maxEnum;
        meta.enumNames.reset(new std::string[maxEnum]);
        meta.enumDisplayInfo.reset(new terra::DisplayInfo[maxEnum]);
        for (int i = 0; i < maxEnum; ++i)
        {
          meta.enumNames[i] = std::get<neo::single>(values[i]).value();
          meta.enumDisplayInfo[i].from(meta.enumNames[i]);
        }
      }
    }
  }

  if (meta.isValid())
  {
    meta.setter = [](terra::Node& node, uint32_t i, terra::Parameter param)
    {
      auto& gpuNode         = static_cast<GpuScriptNode&>(node);
      gpuNode.parameters[i] = param;
    };
    meta.getter = [](terra::Node const& node, uint32_t i) -> Parameter
    {
      auto& gpuNode = static_cast<GpuScriptNode const&>(node);
      return gpuNode.parameters[i];
    };
    builder.meta.parameterDef.emplace_back(std::move(meta));
    return neo::retcode::e_success;
  }
  else
  {
    builder.errorHandler(fmt::format("Parameter info is invalid: {}", meta.name));
    return neo::retcode::e_fail_and_stop;
  }
}

NodeCmdHandler(files, builder, state, cmd)
{
  auto shaders = terra::getFirstList(cmd);
  for (auto& e : shaders)
  {
    builder.content.shaderContent += terra::fileContentToString("shaders/" + e);
    builder.content.shaderContent += "\n";
  }
  return neo::retcode::e_success;
}

NodeCmdHandler(requires, builder, state, cmd)
{
  auto extensionList = terra::getFirstList(cmd);
  for (auto& e : extensionList)
  {
    builder.content.extensions += fmt::format("#extension {} : require\n", e);
  }
  return neo::retcode::e_success;
}

NodeCmdHandler(shaders, builder, state, cmd)
{
  return neo::retcode::e_success;
}

NodeCmdHandler(pass, builder, state, cmd)
{
  return neo::retcode::e_success;
}

NodeCmdHandler(output, builder, state, cmd)
{
  auto type                     = terra::getIdxParam(cmd, 0, "source");
  builder.meta.format.type      = terra::stringToType(type);
  builder.meta.hasTextureOutput = builder.meta.format.type == terra::DataTypeEnum::eImage;
  auto const& params            = cmd.params().value();

  for (auto& p : params)
  {
    if (p.index() != 1)
      continue;
    auto const& entry = std::get<neo::single>(p);
    if (entry.name() == "downscale")
    {
      auto value = entry.value();
      std::from_chars(value.data(), value.data() + value.size(), builder.meta.outputDownscale);
    }
    else if (entry.name() == "type")
    {
      builder.meta.format.scalarSubType = terra::stringToType(entry.value());
    }
    else if (entry.name() == "upscale")
    {
      auto value = entry.value();
      std::from_chars(value.data(), value.data() + value.size(), builder.meta.outputUpscale);
    }
    else if (entry.name() == "format")
    {
      if (entry.value() == "unorm16")
        builder.meta.imageFormat = terra::ImageFormatEnum::eUnorm16;
      else if (entry.value() == "snorm16")
        builder.meta.imageFormat = terra::ImageFormatEnum::eSnorm16;
      else if (entry.value() == "unorm8")
        builder.meta.imageFormat = terra::ImageFormatEnum::eUnorm8;
      else
        builder.meta.imageFormat = terra::ImageFormatEnum::eFloat;
    }
  }
  return neo::retcode::e_success;
}

NodeRegistry(NoiseBuilder)
{
  neo_handle_text(textreg);

  NodeCmd(name);
  NodeCmd(category);
  NodeCmd(function);
  NodeCmd(output);
  NodeCmd(icon);
  NodeCmd(style);
  NodeCmd(param);
  NodeCmd(inject);

  NodeScopeDef(pass)
  {
    NodeCmd(shaders);
    NodeCmd(in);
    NodeCmd(out);
    NodeCmd(requires);
  }
}
