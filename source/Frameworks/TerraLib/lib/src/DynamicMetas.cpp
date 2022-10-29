
#include "NodeMeta.h"
#include "Terra.h"

namespace terra
{

void getMinMax_Node()
{
  auto&    m = get();
  /*
  NodeMeta meta;
  meta.name = m.localizationProvider("@tiledImage");
  meta.icon                        = u8"\xef\x80\xbe";
  meta.help                        = m.localizationProvider("@tiledImageHelp");
  meta.category                    = m.localizationProvider("@DataSource");
  meta.tooltip                     = m.localizationProvider("@tiledImageBrief");
  meta.style                       = "data_src";
  meta.attribIteration             = false;
  meta.attribTileConstrained       = true;
  meta.format.type                 = DataType::eBuffer;
  meta.format.scalarSubType        = DataType::eFloat;
  meta.hasTextureOutput            = true;
  meta.hasUniforms                 = true;
  meta.id                          = "tiledImage";
  meta.imageFormat                 = ImageFormat::eFloat; // dont care

  {
    ParameterMeta source;
    source.name                 = m.localizationProvider("@Source");
    source.format.type          = DataType::eImage;
    source.format.scalarSubType = DataType::eFloat;
    source.help                 = m.localizationProvider("@SourceHelp");
    source.tooltip              = m.localizationProvider("@SourceTooltip");
    source.id                   = "source";

    meta.parameterDef.emplace_back(std::move(source));
  }

  {
    ParameterMeta uvScale;
    uvScale.name        = m.localizationProvider("@UVScale");
    uvScale.format.type = DataType::eFloat2;
    uvScale.help        = m.localizationProvider("@UVScaleHelp");
    uvScale.tooltip     = m.localizationProvider("@UVScaleTooltip");
    uvScale.id          = "uv_scale";

    meta.parameterDef.emplace_back(std::move(uvScale));
  }

  {
    ParameterMeta uvOffset;
    uvOffset.name        = m.localizationProvider("@UVOffset");
    uvOffset.format.type = DataType::eFloat2;
    uvOffset.help        = m.localizationProvider("@UVOffsetHelp");
    uvOffset.tooltip     = m.localizationProvider("@UVOffsetTooltip");
    uvOffset.id          = "uv_offset";

    meta.parameterDef.emplace_back(std::move(uvOffset));
  }

  meta.fillDescriptor = [](Node& node, Pipeline const& pipeline, GfxDescriptorSet::rhandle& rh, std::byte* data)
  {
    rh.first           = handle;
    *(vec2*)data       = vec2{1.f, 1.f};
    *(vec2*)(data + 8) = vec2{0.f, 0.f};
  };*/
}

} // namespace terra