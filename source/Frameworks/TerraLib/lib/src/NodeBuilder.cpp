
#include "NodeBuilder.h"
#include "Node.h"
#include <format>

NodeTextHandler(textreg, build, state, type, name, content) {}

NodeCmdHandler(function, builder, state, cmd)
{
  builder.meta.function = std::string{terra::getIdxParam(cmd, 0)};
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

NodeRegistry(NoiseBuilder)
{
  neo_handle_text(textreg);

  NodeCmd(function);
  NodeCmd(param);
}
