#pragma once

#include "Common.h"

namespace terra
{

template <typename G>
struct Sampler2D
{
  Sampler2D(G* d, int w, int h) : width(w), height(h), buffer(d) {}

  vec2 gradientAt(int x, int y) const
  {
    int idx = y * width + x;
    // int right = y * hmap->width + min(x, hmap->width - 2);
    // int below = min(y, hmap->height - 2) * hmap->width + x;
    int  right = idx + ((x > width - 2) ? 0 : 1);
    int  below = idx + ((y > height - 2) ? 0 : width);
    vec2 g;
    g[0] = buffer[right] - buffer[idx];
    g[1] = buffer[below] - buffer[idx];
    return g;
  }

  vec2 heightGradientAt(vec2 pos) const
  {
    int   x_i      = (int)pos[0];
    int   y_i      = (int)pos[1];
    float u        = pos[0] - (float)x_i;
    float v        = pos[1] - (float)y_i;
    auto  ul       = gradientAt(x_i, y_i);
    auto  ur       = gradientAt(x_i + 1, y_i);
    auto  ll       = gradientAt(x_i, y_i + 1);
    auto  lr       = gradientAt(x_i + 1, y_i + 1);
    auto  ipl_l    = terra::add(scale(1 - v, ul), scale(v, ll));
    auto  ipl_r    = terra::add(scale(1 - v, ur), scale(v, lr));
    auto  gradient = terra::add(scale(1 - u, ipl_l), scale(u, ipl_r));
    return gradient;
  }

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

  inline void remove(float value, int x, int y)
  {
    if (x >= 0 && x < width && y >= 0 && y < height)
      buffer[pixelId(x, y)] -= value;
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