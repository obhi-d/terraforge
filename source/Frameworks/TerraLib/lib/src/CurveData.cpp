
#include "Terra.h"
#include "CurveData.h"

namespace terra
{

bool CurveData::fromDataStream(const std::vector<uint8_t>& dataStream, size_t& serialIdx)
{
  size_t s;

  if (!getFromDataStream(dataStream, serialIdx, s))
    return false;

  std::vector<float> cx, cy;

  cx.resize(s);
  for (auto& vx : cx)
  {
    if (!getFromDataStream(dataStream, serialIdx, vx))
      return false;
  }

  if (!getFromDataStream(dataStream, serialIdx, s))
    return false;
  cy.resize(s);
  for (auto& vy : cy)
  {
    if (!getFromDataStream(dataStream, serialIdx, vy))
      return false;
  }

  tk::spline<>::spline_type type       = tk::spline<>::spline_type::cspline_hermite;
  tk::spline<>::bd_type     left       = tk::spline<>::second_deriv;
  tk::spline<>::bd_type     right      = tk::spline<>::first_deriv;
  float                     leftValue  = 0;
  float                     rightValue = 0;
  bool                      monotonic  = false;
  if (!getFromDataStream(dataStream, serialIdx, left))
    return false;
  if (!getFromDataStream(dataStream, serialIdx, leftValue))
    return false;
  if (!getFromDataStream(dataStream, serialIdx, right))
    return false;
  if (!getFromDataStream(dataStream, serialIdx, rightValue))
    return false;
  if (!getFromDataStream(dataStream, serialIdx, monotonic))
    return false;
  if (!getFromDataStream(dataStream, serialIdx, type))
    return false;

  spline = tk::spline<>::spline(cx, cy, type, monotonic, left, leftValue, right, rightValue);
  bufferDirty = true;
  return true;
}

void CurveData::toDataStream(std::vector<uint8_t>& dataStream) const
{
  auto const& cx = spline.get_x();
  auto const& cy = spline.get_y();
  addToDataStream(dataStream, cx.size());
  for (auto const& vx : cx)
    addToDataStream(dataStream, vx);
  addToDataStream(dataStream, cy.size());
  for (auto const& vy : cy)
    addToDataStream(dataStream, vy);
  auto left       = spline.get_left_deriv();
  auto leftValue  = spline.get_left_value();
  auto right      = spline.get_right_deriv();
  auto rightValue = spline.get_right_value();
  auto monotonic  = spline.is_monotonic();
  auto type       = spline.get_type();

  addToDataStream(dataStream, left);
  addToDataStream(dataStream, leftValue);
  addToDataStream(dataStream, right);
  addToDataStream(dataStream, rightValue);
  addToDataStream(dataStream, monotonic);
  addToDataStream(dataStream, type);
}

void CurveData::ensure() 
{
  if (!handle || bufferDirty)
  {
    get().getDevice().destroy(handle);
    auto size = (1 + 1 + 5 * (uint32_t)spline.get_x().size()) * 4;
    handle = get().getDevice().createBuffer(GfxStorageClass::eStaticDeviceReadonly, GfxBuffer::Usage::fStorage, size);
    std::byte* data = get().getDevice().mapBuffer(handle, 0, size);
    auto       nbpts      = (uint32_t)spline.get_x().size();
    *(float*)(data + 0)   = spline.get_c0();
    *(uint32_t*)(data + 4)= nbpts;
    uint32_t offset        = 8;
    std::memcpy(data + offset, spline.get_x().data(), nbpts * 4);
    offset += nbpts * 4;
    std::memcpy(data + offset, spline.get_y().data(), nbpts * 4);
    offset += nbpts * 4;
    std::memcpy(data + offset, spline.get_b().data(), nbpts * 4);
    offset += nbpts * 4;
    std::memcpy(data + offset, spline.get_c().data(), nbpts * 4);
    offset += nbpts * 4;
    std::memcpy(data + offset, spline.get_d().data(), nbpts * 4);
    get().getDevice().unmapBuffer(handle);
  }
}

CurveData::~CurveData() 
{
  get().getDevice().destroy(handle);
}

}