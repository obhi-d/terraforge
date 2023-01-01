
#include "hyb/GpuScriptNodeBuilder.h"
#include "Node.h"
#include "ResourceUtils.h"
#include "Terra.h"
#include "hyb/HybridNode.h"
#include <fmt/format.h>
#include <format>

using namespace terra;
std::u8string_view terra::GpuScriptNodeBuilder::localizedString(std::string_view name)
{
  return terra::get().localizationProvider(name);
}

NodeTextHandler(textreg, build, state, type, name, content) {}

NodeCmdExecute(name, builder, state, cmd)
{
  builder.meta.displayInfo.from(terra::getIdxParam(cmd, 0));
  return neo::retcode::e_success;
}

NodeCmdExecute(icon, builder, state, cmd)
{
  auto p = terra::getIdxParam(cmd, 0);
  std::from_chars(p.data(), p.data() + p.size(), builder.meta.icon);
  return neo::retcode::e_success;
}

NodeCmdExecute(style, builder, state, cmd)
{
  auto p = terra::getIdxParam(cmd, 0);
  std::from_chars(p.data(), p.data() + p.size(), builder.meta.style);
  return neo::retcode::e_success;
}

NodeCmdExecute(cache, builder, state, cmd)
{
  builder.meta.cacheResults = terra::getIdxParam(cmd, 0) == "true";
  return neo::retcode::e_success;
}

NodeCmdExecute(param, builder, state, cmd)
{
  terra::ParameterMeta meta;

  meta.displayInfo.from(terra::getIdxParam(cmd, 0));
  auto const& params = cmd.params().value();
  for (auto& p : params)
  {
    if (std::holds_alternative<neo::single>(p))
    {
      auto const& entry = std::get<neo::single>(p);
      if (entry.name() == "type")
      {
        meta.setTypeFromString(entry.value(), entry.value());
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
        meta.format.imageFormat = ImageFormat::fromString(entry.value());
      }
      else if (entry.name() == "semantic")
      {
        meta.format.semantic = Semantic::fromString(entry.value());
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
      else if (entry.value() == "hidden")
      {
        meta.format.hidden = true;
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
        meta.maxEnum  = (uint32_t)maxEnum;
        meta.enumDisplayInfo.reset(new terra::DisplayInfo[maxEnum]);
        for (int i = 0; i < maxEnum; ++i)
        {
          meta.enumDisplayInfo[i].from(std::get<neo::single>(values[i]).value());
        }
      }
    }
  }

  if (meta.isValid())
  {
    meta.setter = [](terra::Node& node, uint32_t i, terra::Parameter param)
    {
      auto& gpuNode = static_cast<GpuScriptNode&>(node);
      gpuNode.set(i, param);
    };
    meta.getter = [](terra::Node const& node, uint32_t i) -> Parameter
    {
      auto& gpuNode = static_cast<GpuScriptNode const&>(node);
      return gpuNode.get(i);
    };
    builder.meta.parameterDef.emplace_back(std::move(meta));
    return neo::retcode::e_success;
  }
  else
  {
    builder.errorHandler(fmt::format("Parameter info is invalid: {}", meta.name()));
    return neo::retcode::e_fail_and_stop;
  }
}
NodeCmdExecute(output, builder, state, cmd)
{
  auto        name   = terra::getIdxParam(cmd, 0, "source");
  auto        output = OutputMeta(std::string(name));
  auto const& params = cmd.params().value();
  for (auto& p : params)
  {
    if (std::holds_alternative<neo::single>(p))
    {
      auto const& entry = std::get<neo::single>(p);
      if (entry.name() == "type")
      {
        output.format.scalarSubType = output.format.type = DataType::fromString(entry.value());
      }
      else if (entry.name() == "decltype")
      {
        output.format.declType = ParamDeclType::fromString(entry.value());
      }
      else if (entry.name() == "preeval")
      {
        output.format.preEval = true;
      }
      else if (entry.name() == "format")
      {
        output.format.imageFormat = ImageFormat::fromString(entry.value());
      }
      else if (entry.name() == "semantic")
      {
        output.format.semantic = Semantic::fromString(entry.value());
      }
      else if (entry.name() == "clear")
      {
        output.clear = true;
        if (!entry.value().empty())
        {
          std::from_chars(entry.value().data(), entry.value().data() + entry.value().size(), output.clearValue.x);
        }
      }
    }
    else if (std::holds_alternative<neo::list>(p))
    {
      auto const& entry = std::get<neo::list>(p);
      if (entry.name() == "type")
      {
        auto const& values          = entry.value();
        output.format.type          = DataType::fromString(std::get<neo::single>(values[0]).value());
        output.format.scalarSubType = DataType::fromString(std::get<neo::single>(values[1]).value());
      }
      else if (entry.name() == "clear")
      {
        output.clear = true;
        if (!entry.value().empty())
        {
          auto const& values = entry.value();
          uint32_t    nv     = std::min<uint32_t>((uint32_t)entry.value().size(), 4);
          for (uint32_t i = 0; i < nv; ++i)
          {
            auto v = std::get<neo::single>(entry.value().at(i)).value();
            std::from_chars(v.data(), v.data() + v.size(), output.clearValue[i]);
          }
        }
      }
    }
  }
  builder.meta.outputs.emplace_back(output);
  return neo::retcode::e_success;
}

NodeCmdExecute(pass, builder, state, cmd)
{
  builder.meta.passes.emplace_back();
  builder.meta.passes.back().function = std::string(terra::getIdxParam(cmd, 0, "main"));
  return neo::retcode::e_success;
}

NodeCmdExecute(function, builder, state, cmd)
{
  builder.meta.passes.back().function = terra::getIdxParam(cmd, 0);
  return neo::retcode::e_success;
}

NodeCmdExecute(shaders, builder, state, cmd)
{
  auto shaders = terra::getFirstList(cmd);
  for (auto& e : shaders)
  {
    builder.meta.passes.back().shaderContent += terra::fileContentToString("shaders/" + e);
    builder.meta.passes.back().shaderContent += "\n";
  }
  return neo::retcode::e_success;
}

NodeCmdExecute(extensions, builder, state, cmd)
{
  auto extensionList = terra::getFirstList(cmd);
  for (auto& e : extensionList)
  {
    builder.meta.passes.back().extensions += fmt::format("#extension {} : require\n", e);
  }
  return neo::retcode::e_success;
}

NodeCmdExecute(in, builder, state, cmd)
{
  auto     names = terra::getFirstList(cmd);
  uint32_t s     = 0;
  for (auto& e : names)
  {
    [&]()
    {
      for (uint32_t i = 0, end = (uint32_t)builder.meta.parameterDef.size(); i != end; ++i)
      {
        uint32_t d = s + i;
        uint32_t v = d % end;
        if (builder.meta.parameterDef[v].name() == e)
        {
          builder.meta.passes.back().parameters.emplace_back(v);
          s = v;
          return;
        }
      }
    }();
  }
  return neo::retcode::e_success;
}

NodeCmdExecute(out, builder, state, cmd)
{
  auto names = terra::getFirstList(cmd);
  for (auto& e : names)
  {
    for (uint32_t i = 0, end = (uint32_t)builder.meta.outputs.size(); i != end; ++i)
      if (builder.meta.outputs[i].name() == e)
      {
        builder.meta.passes.back().outputs.emplace_back(i);
        break;
      }
  }
  return neo::retcode::e_success;
}

NodeRegistry(GpuScript)
{
  neo_handle_text(textreg);

  NodeCmd(name);
  NodeCmd(output);
  NodeCmd(icon);
  NodeCmd(style);
  NodeCmd(param);

  NodeScopeDef(pass)
  {
    NodeCmd(function);
    NodeCmd(shaders);
    NodeCmd(in);
    NodeCmd(out);
    NodeCmd(extensions);
  }
}
