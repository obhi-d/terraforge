
#include "hwy/NodeMeta_hwy.h"
#include "Terra.h"
#include "hwy/Pipeline_hwy.h"

namespace terra
{

inline void NodeMeta_hwy::fill(float value, Pipeline_hwy& pipe, uint32_t threadGroupId, uint32_t lanes)
{
  auto& out_a = pipe.getOutput(threadGroupId, lanes);
  out_a.fill(value);
}

void NodeMeta_hwy::write(Parameter const& param, Pipeline_hwy& pipe, uint32_t threadGroupId, uint32_t lanes)
{
  if (std::holds_alternative<Source>(param))
  {
    auto src = std::get<Source>(param).source;
    if (DataSource::isValid(src))
    {
      run(src, pipe, threadGroupId, lanes);
    }
    else
    {
      fill(0.0f, pipe, threadGroupId, lanes);
    }
  }
  else
  {
    fill(std::get<ScalarValue>(param).value, pipe, threadGroupId, lanes);
  }
}

void NodeMeta_hwy::domain(Parameter const& param, Pipeline_hwy& pipe, uint32_t threadGroupId, uint32_t lanes)
{
  if (std::holds_alternative<Source>(param))
  {
    auto src = std::get<Source>(param).source;
    if (DataSource::isValid(src))
    {
      run(src, pipe, threadGroupId, lanes);
    }
  }
}

void NodeMeta_hwy::run(dshandle h, Pipeline_hwy& pipe, uint32_t threadGroupId, uint32_t lanes)
{
  assert(DataSource::isValid(h));

  auto&       node = get().get<Node>(h);
  auto const& meta = (NodeMeta_hwy const&)node.meta;
  meta.fn(node, pipe, threadGroupId);
}

void NodeMeta_hwy::prepareGeneration(Node& node, Pipeline& pipe) const
{
  if (prepare)
    prepare(node, (Pipeline_hwy&)pipe);
}

void NodeMeta_hwy::beginIteration(Node& node, Pipeline& pipe) const
{
  if (beginIt)
    beginIt(node, (Pipeline_hwy&)pipe);
}
void NodeMeta_hwy::endIteration(Node& node, Pipeline& pipe) const
{
  if (endIt)
    endIt(node, (Pipeline_hwy&)pipe);
}

} // namespace terra