
#pragma once
#include "GlGfx.h"
#include <string_view>

namespace terra
{
inline gl::GLenum toGlFormat(ImageFormat format)
{
  switch (format)
  {
  case ImageFormat::eFloat:
    return gl::GLenum::GL_R32F;
  case ImageFormat::eUnorm8:
    return gl::GLenum::GL_R8;
  case ImageFormat::eSnorm16:
    return gl::GLenum::GL_R16_SNORM;
  case ImageFormat::eUnorm16:
    return gl::GLenum::GL_R16;
  case ImageFormat::eRgba8:
    return gl::GLenum::GL_RGBA8;
  case ImageFormat::eSrgb8Alpha8:
    return gl::GLenum::GL_SRGB8_ALPHA8;
  case ImageFormat::eRgb32f:
    return gl::GLenum::GL_RGB32F;
  case ImageFormat::eRgba32f:
    return gl::GLenum::GL_RGBA32F;
  }
  return gl::GLenum::GL_NONE;
}
inline gl::GLenum toGlDataFormat(ImageFormat format)
{
  switch (format)
  {
  case ImageFormat::eFloat:
  case ImageFormat::eUnorm8:
  case ImageFormat::eSnorm16:
  case ImageFormat::eUnorm16:
    return gl::GLenum::GL_RED;
  case ImageFormat::eRgba8:
  case ImageFormat::eSrgb8Alpha8:
  case ImageFormat::eRgba32f:
    return gl::GLenum::GL_RGBA;
  case ImageFormat::eRgb32f:
    return gl::GLenum::GL_RGB;
  }
  return gl::GLenum::GL_NONE;
}
inline gl::GLenum toGlType(ImageFormat format)
{
  switch (format)
  {
  case ImageFormat::eRgb32f:
  case ImageFormat::eRgba32f:
  case ImageFormat::eFloat:
    return gl::GLenum::GL_FLOAT;
  case ImageFormat::eSnorm16:
    return gl::GLenum::GL_SHORT;
  case ImageFormat::eUnorm16:
    return gl::GLenum::GL_UNSIGNED_SHORT;
  case ImageFormat::eUnorm8:
  case ImageFormat::eRgba8:
  case ImageFormat::eSrgb8Alpha8:
    return gl::GLenum::GL_UNSIGNED_BYTE;
  }
  return gl::GLenum::GL_NONE;
}
inline gl::GLenum toGlType(ShaderType type)
{
  switch (type)
  {
  case ShaderType::eCompute:
    return gl::GL_COMPUTE_SHADER;
  case ShaderType::eVertex:
    return gl::GL_VERTEX_SHADER;
  case ShaderType::eFragment:
    return gl::GL_FRAGMENT_SHADER;
  }
  return gl::GLenum::GL_NONE;
}
inline gl::GLenum toGLDrawMode(GfxMesh::Type type)
{
  switch (type)
  {
  case GfxMesh::eLines:
    return gl::GL_LINES;
  case GfxMesh::eTriangles:
    return gl::GL_TRIANGLES;
  case GfxMesh::ePoints:
    return gl::GL_POINTS;
  }
  return gl::GLenum::GL_NONE;
}

inline std::string_view toString(Access access)
{
  return access == Access::eReadonly ? "readonly" : (access == Access::eWriteonly ? "writeonly" : "");
}

inline auto toGlMapBit(Access access)
{
  return access == Access::eReadonly ? gl::GL_READ_ONLY
                                     : (access == Access::eWriteonly ? gl::GL_WRITE_ONLY : gl::GL_READ_WRITE);
}

inline auto toGl(GfxImage2D::ComponentValue comp)
{
  switch (comp)
  {
  case GfxImage2D::ComponentValue::eRed:
    return gl::GL_RED;
  case GfxImage2D::ComponentValue::eGreen:
    return gl::GL_GREEN;
  case GfxImage2D::ComponentValue::eBlue:
    return gl::GL_BLUE;
  case GfxImage2D::ComponentValue::eAlpha:
    return gl::GL_ALPHA;
  case GfxImage2D::ComponentValue::eZero:
    return gl::GL_ZERO;
  default:
    return gl::GL_ONE;
  }
}
} // namespace terra