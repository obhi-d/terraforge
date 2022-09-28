#include "Terra.h"
#include "NodeBuilder.h"

#include <format>
#include <fstream>

neo_registry(NoiseBuilder);
namespace terra
{

void Terra::init(std::shared_ptr<RenderDevice> dev)
{
  device = dev;
  NodeRegister(NoiseBuilder, registry);
}

void Terra::scanShader(std::filesystem::path path)
{
  std::ifstream iff(path);
  if (iff.is_open())
  {
    NodeMeta           newMeta;
    NodeCmdHandler     handler(newMeta, *this,
                               [this](std::string err)
                               {
                             logError(err);
                           });
    neo::state_machine sm{registry, &handler};

    std::string f1_str((std::istreambuf_iterator<char>(iff)), std::istreambuf_iterator<char>());
    sm.parse(path.string(), f1_str);
    if (!sm.fail_bit())
    {
      if (!newMeta.buildShader(*this, *device))
        logError(std::format("Failed to build shader: {}", path.string()));

      meta[newMeta.category].emplace(newMeta.name, newMeta);
    }
  }
  else
  {
    logError(std::format("Failed to open file: {}", path.string()));
  }
}

} // namespace terra