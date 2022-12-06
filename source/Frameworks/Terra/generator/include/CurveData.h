#pragma once

#include "DataSource.h"
#include "GfxDevice.h"
#include "RenderResource.h"
#include "Serializer.h"
#include "spline.h"
#include <future>

namespace terra
{
class Terra;
struct CurveData : public DataSource
{
  struct Edit
  {
    std::vector<float>        cx              = {0.0f, 0.5f, 1.0f};
    std::vector<float>        cy              = {0.0f, 0.5f, 1.0f};
    tk::spline<>::spline_type type            = tk::spline<>::spline_type::cspline_hermite;
    tk::spline<>::bd_type     left            = tk::spline<>::second_deriv;
    tk::spline<>::bd_type     right           = tk::spline<>::first_deriv;
    float                     leftValue       = 0;
    float                     rightValue      = 1.0f;
    int                       dragged         = -1;
    bool                      popupType       = false;
    bool                      popupLeftBound  = false;
    bool                      popupRightBound = false;
    bool                      monotonic       = true;
    bool                      edited          = false;
    bool                      dirty           = false;
    bool                      firstEdit       = true;
    bool                      liveUpdate      = false;
    tk::spline<>              spline;
  };

  std::u8string name = u8"CurveData";
  tk::spline<>  spline;
  Edit          edits;
  uint32        version = 0;

  CurveData()
  {
    tk::spline<>::spline_type type       = tk::spline<>::spline_type::cspline_hermite;
    tk::spline<>::bd_type     left       = tk::spline<>::second_deriv;
    tk::spline<>::bd_type     right      = tk::spline<>::first_deriv;
    float                     leftValue  = 0;
    float                     rightValue = 1.0f;
    bool                      monotonic  = true;
    std::vector<float>        cx         = {0.0f, 0.5f, 1.0f};
    std::vector<float>        cy         = {0.0f, 0.5f, 1.0f};

    spline = tk::spline<>(cx, cy, type, monotonic, left, leftValue, right, rightValue);
  }
  ~CurveData();

  void beginEdit()
  {
    if (edits.edited)
      return;
    edits.cx         = spline.get_x();
    edits.cy         = spline.get_y();
    edits.type       = spline.get_type();
    edits.left       = spline.get_left_deriv();
    edits.right      = spline.get_right_deriv();
    edits.leftValue  = spline.get_left_value();
    edits.rightValue = spline.get_right_value();
    edits.monotonic  = edits.firstEdit ? spline.is_monotonic() : edits.monotonic;
    edits.dragged    = -1;
    edits.edited     = true;
    edits.dirty      = false;
    edits.firstEdit  = false;
    edits.spline     = spline;
  }

  bool endEdits(bool apply)
  {
    if (edits.dirty)
    {
      edits.spline = tk::spline<>(edits.cx, edits.cy, edits.type, edits.monotonic, edits.left, edits.leftValue,
                                  edits.right, edits.rightValue);
    }
    if ((edits.liveUpdate || apply) && edits.edited && edits.dirty)
    {
      spline  = edits.spline;
      version = (self.index() << 16) | version++;
      if (apply)
      {
        edits.edited  = false;
        edits.dirty   = false;
        edits.dragged = -1;
      }
      return true;
    }
    return false;
  }

  inline bool operator==(const CurveData& other) const
  {
    return spline == other.spline;
  }

  bool getBuffer(GfxDevice&, uint32& version, GfxBuffer::handle& ioBuffer);

  inline Type getType() const final
  {
    return Type::eCurve;
  }

  inline DataFormat getFormat(uint32) const final
  {
    return DataFormat(DataTypeEnum::eCurveData);
  }

  inline exchange setParamSourceImpl(uint32_t paramIdx, Source) final
  {
    return exchange({}, false);
  }

  // bool isEnabled(Pipeline const&) const final;

  inline void accept(Source source, Event) final {}

  void     getSourcesImpl(SourceSet& s) const final {}
  bool     fromDataStreamImpl(const std::vector<uint8_t>& dataStream, size_t& serialIdx) final;
  void     toDataStreamImpl(std::vector<uint8_t>& dataStream) const final;
  HelpInfo getHelpInfo(HelpType, int param = -1) const final;
};

using CurveDataPtr = std::shared_ptr<CurveData>;

} // namespace terra
