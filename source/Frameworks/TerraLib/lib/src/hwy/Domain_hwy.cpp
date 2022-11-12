#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "Noise_hwy.cpp"

#include "Common.h"
#include "Node.h"
#include <hwy/foreach_target.h>

#include "hwy/NodeMeta_hwy.h"
#include "hwy/Pipeline_hwy.h"
#include "hwy/Utility_hwy.h"

#include "Terra.h"

HWY_BEFORE_NAMESPACE();
namespace terra::HWY_NAMESPACE
{
namespace hn = hwy::HWY_NAMESPACE;

void domainRotate(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId) 
{}

void domainScale(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId) {}
void domainOffset(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId) {}
void domainWarp(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId) {}

} // namespace terra::HWY_NAMESPACE
HWY_AFTER_NAMESPACE();

#if HWY_ONCE

namespace terra
{}