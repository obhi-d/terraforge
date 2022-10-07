
#include "Terra.h"

namespace terra
{
class ImguiBackend;
class NodeEditor
{
public:
  void scanNodeMetas();
  void drawNodeEditor(ImguiBackend&);
};
} // namespace terra