
#include "Terra.h"
#include "hwy/Pipeline_hwy.h"
#include "hwy/NodeMeta_hwy.h"

namespace terra
{

void NodeMeta_hwy::fill(float value, Pipeline_hwy& pipe, uint32_t threadGroupId, uint32_t lanes)
{
  pipe.getOutput(threadGroupId, lanes);
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

void NodeMeta_hwy::run(dshandle h, Pipeline_hwy& pipe, uint32_t threadGroupId, uint32_t lanes) 
{
  assert(DataSource::isValid(h));
  
  auto& node = get().get<Node>(h);
  auto const& meta = (NodeMeta_hwy const&)node.meta;
  meta.fn(node, pipe, threadGroupId);
}

}