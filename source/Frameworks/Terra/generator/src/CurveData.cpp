
#include "Terra.h"
#include "CurveData.h"

namespace terra
{

bool CurveData::fromDataStreamImpl(const std::vector<uint8_t>& dataStream, size_t& serialIdx)
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

  spline = tk::spline<>(cx, cy, type, monotonic, left, leftValue, right, rightValue);
  version = (self.index() << 16) | version++;
  return true;
}

void CurveData::toDataStreamImpl(std::vector<uint8_t>& dataStream) const
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

bool CurveData::getBuffer(ComputeDevice& dev, uint32& bufferVer, GfxBuffer::handle& handle)
{
  if (!handle || version != bufferVer)
  {
    dev.destroy(handle);
    auto size = (1 + 1 + 5 * (uint32_t)spline.get_x().size()) * 4;
    handle = dev.createBuffer(GfxStorageClass::eStaticDeviceReadonly, GfxBuffer::Usage::fStorage, size);
    if (!handle)
      return false;
    ubyte_t* data = dev.mapBuffer(handle, 0, size);
    if (!data)
      return false;
    auto     nbpts  = (uint32_t)spline.get_x().size();
    uint32_t offset        = 0;
    std::memcpy(data + offset, spline.get_x().data(), nbpts * 4);
    offset += nbpts * 4;
    std::memcpy(data + offset, spline.get_y().data(), nbpts * 4);
    offset += nbpts * 4;
    std::memcpy(data + offset, spline.get_b().data(), nbpts * 4);
    offset += nbpts * 4;
    std::memcpy(data + offset, spline.get_c().data(), nbpts * 4);
    offset += nbpts * 4;
    std::memcpy(data + offset, spline.get_d().data(), nbpts * 4);
    dev.unmapBuffer(handle);
    bufferVer = version;
    return true;
  }
  return false;
}

HelpInfo CurveData::getHelpInfo(HelpType type, int param) const 
{
  static HelpInfo output ("@curveOut.help"_ls, "@curveOut.tip"_ls );
  static HelpInfo main("@curve.help"_ls, "@curve.tip"_ls);
  switch (type)
  {
  case HelpType::eDataSource:
    return main;
  case HelpType::eOutput:
    return output;
  }
  return {};
}

CurveData::~CurveData() 
{
}

}
