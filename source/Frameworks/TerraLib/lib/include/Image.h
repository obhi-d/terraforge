#pragma once

#include "DataSource.h"
#include "RenderResource.h"
#include "Serializer.h"
#include <filesystem>
#include <memory>

namespace terra
{
class Terra;

struct Image : public DataSource
{
  std::filesystem::path        source;
  std::unique_ptr<std::byte[]> data;
  uint32_t                     width  = 0;
  uint32_t                     height = 0;
  ImageFormat                  format = ImageFormat::eFloat;

  struct rgba
  {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
  };

  Image() = default;
  Image(std::filesystem::path path) : source(std::move(path))
  {
    load();
  }
  Image(Image const&)            = default;
  Image(Image&&)                 = default;
  Image& operator=(Image const&) = default;
  Image& operator=(Image&&)      = default;
  ~Image()
  {
    unload();
  }

  Type getType() const final
  {
    return Type::eImage;
  }

  DataFormat getFormat() const final
  {
    return DataFormat(DataType::eImage);
  }

  inline std::pair<dshandle, bool> setParamSourceImpl(uint32_t paramIdx, Source) final
  {
    return std::make_pair<dshandle, bool>({}, false);
  }

  template <typename T>
  T& get(int i, int j)
  {
    return *(T*)(data.get() + ((static_cast<uint32_t>(j) * width + static_cast<uint32_t>(i)) * sizeof(T)));
  }

  template <typename T>
  T get(int i, int j) const
  {
    return *(T const*)(data.get() + ((static_cast<uint32_t>(j) * width + static_cast<uint32_t>(i)) * sizeof(T)));
  }

  inline float sample_val(int x, int y) const
  {
    switch (format)
    {
    case ImageFormat::eFloat:
      return (float)get<float>(x, y);
    case ImageFormat::eUnorm8:
      return (float)get<std::uint8_t>(x, y) / 255.f;
    case ImageFormat::eSnorm16:
    case ImageFormat::eUnorm16:
      return (float)((float)get<std::uint16_t>(x, y) / (float)std::numeric_limits<std::uint16_t>::max());
    case ImageFormat::eRgba8:
    case ImageFormat::eSrgb8Alpha8:
    {
      auto rgb = get<rgba>(x, y);
      return (float)(.299f * ((float)rgb.r / 255.f) + .587f * ((float)rgb.g / 255.f) + .114f * ((float)rgb.b / 255.f));
    }
      
    }
    return 0.f;
  }

  inline float sample(float u, float v) const
  {
    auto x = std::max<int>(0, std::min<int>((int)(u * ((float)width - 0.5f)), width - 1));
    auto y = std::max<int>(0, std::min<int>((int)(v * ((float)height - 0.5f)), height - 1));
    return sample_val(x, y);
  }

  template <int N>
  inline float sampleN(float u, float v) const
  {
    auto            cx    = (int)(u * ((float)width - 0.5f));
    auto            cy    = (int)(v * ((float)height - 0.5f));
    float           value = 0;
    constexpr float recip = 1.f / (N * N * 4.f);

    for (int sy = -N; sy < N; ++sy)
    {
      for (int sx = -N; sx < N; ++sx)
      {
        auto x = std::max<int>(0, std::min<int>(cx + sx, width - 1));
        auto y = std::max<int>(0, std::min<int>(cy + sy, height - 1));
        value += sample_val(x, y);
      }
    }

    value *= recip;
    return value;
  }

  inline bool isLoaded() const
  {
    return data != nullptr;
  }

  void        unload();
  bool        load();
  void        reload()
  {
    unload();
    load();
  }
  // bool        isEnabled(Pipeline const&) const final;
  // bool        ensure(Pipeline&) final;
  // void     remove(dshandle node) final;
  bool     fromDataStreamImpl(const std::vector<uint8_t>& dataStream, size_t& serialIdx) final;
  void     toDataStreamImpl(std::vector<uint8_t>& dataStream) const;
  HelpInfo getHelpInfo(HelpType type, int param = -1) const final;
  void     getSourcesImpl(SourceSet& s) const final {}
};

using ImagePtr = std::shared_ptr<Image>;

class Node;

} // namespace terra