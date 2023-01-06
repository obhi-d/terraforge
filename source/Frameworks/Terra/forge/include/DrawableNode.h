
#pragma once
#include "ImguiTheme.h"
#include "Setup.h"
#include "Terra.h"
#include "imgui.h"
#include "imgui_node_editor.h"

namespace imne = ax::NodeEditor;

namespace terra
{

class ImguiBackend;
class TerraMainApp;

struct PinData
{
  struct Input
  {};
  struct Output
  {};

  static inline constexpr Input  input  = {};
  static inline constexpr Output output = {};

  PinData() = default;
  PinData(Input, HDataSource src, uint16_t idx) : src_(src), id_(idx), valid_(DataSource::isValid(src)) {}
  PinData(Output, HDataSource src, uint16_t idx)
      : src_(src), id_(idx), isOutput_(true), valid_(DataSource::isValid(src))
  {}
  PinData(imne::PinId p)
  {
    *this = p;
  }

  inline auto operator<=>(PinData const&) const noexcept = default;

  inline bool isValid() const
  {
    return valid_;
  }

  inline auto id() const
  {
    return id_;
  }

  inline HDataSource src() const
  {
    return src_;
  }

  inline PinData& operator=(imne::PinId p) noexcept
  {
    auto [src, id] = unpack(p.Get());
    src_           = HDataSource{src};
    id_            = uint16_t{id & 0xffff};
    isOutput_      = (id & 0x80000000) != 0;
    valid_         = DataSource::isValid(src_);
    return *this;
  }

  inline bool isOutput() const noexcept
  {
    return isOutput_;
  }

  imne::PinId pinId() const noexcept
  {
    uint32_t id = id_;
    if (isOutput())
      id |= 0x80000000;
    return imne::PinId(pack(src_.um_index(), id));
  }

  HDataSource src_;
  uint16_t    id_       = 0xffff;
  bool        isOutput_ = false;
  bool        valid_    = false;
};

struct NodeStyle;
// enum class PinStateFlags
//{
//   fNone        = 0,
//   fShowTooltip = 1 << 0,
//   fShowHelp    = 1 << 1,
//   fOutput      = 1 << 2,
//   fInputPin    = 1 << 3,
//   fIsFilled    = 1 << 4,
// };
class NodeEditor;
class DrawableNode
{
public:
  inline static constexpr uint32_t IsExecuting = 1;
  inline static constexpr uint32_t IsSelected  = 2;

  DrawableNode(TerraMainApp&, HDataSource id, ImVec2 pos);
  ~DrawableNode();
  bool begin(TerraMainApp&, ImguiBackend&, NodeEditor&, uint32_t styleFlags);
  void end(TerraMainApp&, ImguiBackend&, NodeEditor&, uint32_t styleFlags);
  bool is(HDataSource id) const
  {
    return id == this->id;
  }

  HDataSource getId() const
  {
    return id;
  }

  void updateThumbnailFromImage(Image&);

private:
  void drawPinIcon(NodeEditor&, NodeStyle const&, imne::PinId id, const char* name, DataFormat type, bool output,
                   bool detached);
  void drawHeader(NodeEditor&, NodeStyle const&, ImVec2 headerMin, ImVec2 headerMax);
  void drawParameter(NodeEditor&, NodeStyle const&, Node&, uint32_t);

  std::vector<PinData> parameters;
  std::vector<PinData> outputs;

  float width = 0.0f;

  HDataSource id;
  uint32_t    style = 0;

  ImVec2 pos{};

  static inline float constexpr ThumbnailSize = 200.f;
  GfxImage::handle thumbnail;
  uint32_t         thumbnailVersion = std::numeric_limits<uint32_t>::max();

  bool firstDraw = true;
};

struct Link
{
  imne::LinkId id;
  Color        color;
  PinData      start;
  PinData      end;
};
} // namespace terra