
#include "ResourceUtils.h"
#include "NodeBuilder.h"
#include "Node.h"
#include "Terra.h"
#include <format>


std::u8string_view terra::NodeCmdHandler::localizedString(std::string_view name)
{
  return terra::get().localizationProvider(name);
}

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
  builder.content.function = terra::getIdxParam(cmd, 0);
  return neo::retcode::e_success;
}

NodeCmdHandler(iteration, builder, state, cmd)
{
  auto value = terra::getIdxParam(cmd, 0);
  if (value == "inf")
    builder.meta.iteration = std::numeric_limits<uint32_t>::max();
  else
    std::from_chars(value.data(), value.data() + value.length(), builder.meta.iteration);
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
    else if (entry.name() == "hint")
    {
      if (entry.value() == "sameline")
        meta.drawHint = terra::DrawHint::eSameline;
      else if (entry.value() == "hidden")
        meta.drawHint = terra::DrawHint::eHidden;
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
    builder.content.extensions += "#extension ";
    builder.content.extensions += e;
    builder.content.extensions += " : require\n";
  }
  return neo::retcode::e_success;
}

NodeCmdHandler(config, builder, state, cmd)
{
  if (!terra::get().isShaderConfigSupported(terra::getIdxParam(cmd, 0)))
    return neo::retcode::e_skip_block;
  return neo::retcode::e_success;
}

NodeCmdHandler(shaders, builder, state, cmd)
{
  return neo::retcode::e_success;
}

NodeCmdHandler(type, builder, state, cmd)
{
  auto type = terra::getIdxParam(cmd, 0, "height");
  if (type == "height")
    builder.meta.hasTextureOutput = false;
  else if (type == "image")
    builder.meta.hasTextureOutput = false;
  auto const& params = cmd.params().value();
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
    else if (entry.name() == "upscale")
    {
      auto value = entry.value();
      std::from_chars(value.data(), value.data() + value.size(), builder.meta.outputUpscale);
    }
    else if (entry.name() == "format")
    {
      if (entry.value() == "unorm16")
        builder.meta.imageFormat = terra::ImageFormat::eUnorm16;
      else if (entry.value() == "snorm16")
        builder.meta.imageFormat = terra::ImageFormat::eSnorm16;
      else
        builder.meta.imageFormat = terra::ImageFormat::eFloat;
    }
  }
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
  NodeCmd(type);
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
