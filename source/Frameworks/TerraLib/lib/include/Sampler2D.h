#pragma once

namespace terra
{

template <typename G>
struct Sampler2D
{
  Sampler2D(G* d, int w, int h) : width(w), height(h), buffer(d) {}

  inline int pixelIdWarped(int x, int y) const
  {
    return ((unsigned int)x % width) + ((unsigned int)y % height) * width;
  }

  inline int pixelId(int x, int y) const
  {
    return x + y * width;
  }

  inline void add(float value, int x, int y)
  {
    if (x >= 0 && x < width && y >= 0 && y < height)
      buffer[pixelId(x, y)] += value;
  }

  inline void madd(float value, float scale, int x, int y)
  {
    if (x >= 0 && x < width && y >= 0 && y < height)
    {
      auto& v = buffer[pixelId(x, y)];
      v *= scale;
      v += value;
    }
  }

  inline auto sampleUnsafe(int x, int y) const
  {
    return buffer[pixelId(x, y)];
  }

  inline auto sample(int x, int y) const
  {
    return buffer[pixelIdWarped(x, y)];
  }

  inline auto sampleSafe(int x, int y) const
  {
    if (x >= 0 && x < width && y >= 0 && y < width)
      return buffer[pixelId(x, y)];
    else
      return 0.0f;
  }

  inline auto bisample(float x, float y) const
  {
    int   xi = (int)x;
    int   yi = (int)y;
    float u  = x - xi;
    float v  = y - yi;
    float d0 = sampleSafe(xi, yi);
    float d1 = sampleSafe(xi + 1, yi);
    float d2 = sampleSafe(xi, yi + 1);
    float d3 = sampleSafe(xi + 1, yi + 1);
    return (((1 - v) * d0 + v * d2) * (1 - u)) + (u * ((1 - v) * d1 + v * d3));
  }

  inline auto& at(int x, int y)
  {
    return buffer[pixelId(x, y)];
  }

  inline bool isInBounds(int x, int y)
  {
    return x >= 0 && x < width && y >= 0 && y < height;
  }

  int width;
  int height;
  G*  buffer;
};

} // namespace terra