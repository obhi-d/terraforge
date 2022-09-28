
#include "NodeBuilder.h"
#include "Node.h"
#include "Terra.h"
#include <format>

NodeTextHandler(textreg, build, state, type, name, content) {}

NodeCmdHandler(name, builder, state, cmd)
{
  builder.meta.name = builder.localizedString(terra::getIdxParam(cmd, 0));
  return neo::retcode::e_success;
}

NodeCmdHandler(help, builder, state, cmd)
{
  builder.meta.help = builder.localizedString(terra::getIdxParam(cmd, 0));
  return neo::retcode::e_success;
}

NodeCmdHandler(category, builder, state, cmd)
{
  builder.meta.category = builder.localizedString(terra::getIdxParam(cmd, 0));
  return neo::retcode::e_success;
}

NodeCmdHandler(brief, builder, state, cmd)
{
  builder.meta.brief = builder.localizedString(terra::getIdxParam(cmd, 0));
  return neo::retcode::e_success;
}

NodeCmdHandler(function, builder, state, cmd)
{
  builder.meta.function = terra::getIdxParam(cmd, 0);
  return neo::retcode::e_success;
}

NodeCmdHandler(param, builder, state, cmd)
{
  terra::ParameterMeta meta;
  meta.name          = std::string{terra::getIdxParam(cmd, 0)};
  auto const& params = cmd.params().value();
  for (auto& p : params)
  {
    if (p.index() != 1)
      continue;
    auto const& entry = std::get<neo::single>(p);
    if (entry.name() == "type")
    {
      meta.setTypeFromString(entry.value());
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
    else if (entry.name() == "placement")
    {
      if (entry.value() == "sameline")
        meta.drawHint = terra::DrawHint::eSameline;
    }
  }
  if (meta.isValid())
  {
    builder.meta.parameterDef.emplace_back(meta);
    return neo::retcode::e_success;
  }
  else
  {
    builder.errorHandler(std::format("Parameter info is invalid: {}", meta.name));
    return neo::retcode::e_fail_and_stop;
  }
}

NodeCmdHandler(files, builder, state, cmd)
{
  auto shaders = terra::getFirstList(cmd);
  for (auto& e : shaders)
  {
    builder.meta.shaderContent += builder.controller.loadMediaString("shaders/" + e);
    builder.meta.shaderContent += "\n";
  }
  return neo::retcode::e_success;
}

NodeCmdHandler(requires, builder, state, cmd)
{
  auto extensionList = terra::getFirstList(cmd);
  for (auto& e : extensionList)
  {
    builder.meta.extensions += "#extension ";
    builder.meta.extensions += e;
    builder.meta.extensions += " : require\n";
  }
  return neo::retcode::e_success;
}

NodeCmdHandler(config, builder, state, cmd)
{
  if (!builder.controller.isShaderConfigSupported(terra::getIdxParam(cmd, 0)))
    return neo::retcode::e_skip_block;
}

NodeCmdHandler(shaders, builder, state, cmd)
{
  return neo::retcode::e_success;
}

NodeRegistry(NoiseBuilder)
{
  neo_handle_text(textreg);

  NodeCmd(name);
  NodeCmd(help);
  NodeCmd(category);
  NodeCmd(brief);
  NodeCmd(function);
  NodeScopeDef(shaders)
  {
    NodeScopeDef(config)
    {
      NodeCmd(files);
      NodeCmd(requires);
    }
  }

  NodeCmd(param);
}
