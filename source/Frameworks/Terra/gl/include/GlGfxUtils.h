
#pragma once
#include "GlGfx.h"
#include <string_view>

namespace terra
{
inline gl::GLenum toGlFormat(ImageFormatEnum format)
{
  switch (format)
  {
  case ImageFormatEnum::eFloat:
    return gl::GLenum::GL_R32F;
  case ImageFormatEnum::eUnorm8:
    return gl::GLenum::GL_R8;
  case ImageFormatEnum::eSnorm16:
    return gl::GLenum::GL_R16_SNORM;
  case ImageFormatEnum::eUnorm16:
    return gl::GLenum::GL_R16;
  case ImageFormatEnum::eRgba8:
    return gl::GLenum::GL_RGBA8;
  case ImageFormatEnum::eSrgb8Alpha8:
    return gl::GLenum::GL_SRGB8_ALPHA8;
  case ImageFormatEnum::eRg32f:
    return gl::GLenum::GL_RG32F;
  case ImageFormatEnum::eRgba32f:
    return gl::GLenum::GL_RGBA32F;
  }
  return gl::GLenum::GL_NONE;
}
inline gl::GLenum toGlDataFormat(ImageFormatEnum format)
{
  switch (format)
  {
  case ImageFormatEnum::eFloat:
  case ImageFormatEnum::eUnorm8:
  case ImageFormatEnum::eSnorm16:
  case ImageFormatEnum::eUnorm16:
    return gl::GLenum::GL_RED;
  case ImageFormatEnum::eRgba8:
  case ImageFormatEnum::eSrgb8Alpha8:
  case ImageFormatEnum::eRgba32f:
    return gl::GLenum::GL_RGBA;
  case ImageFormatEnum::eRg32f:
    return gl::GLenum::GL_RG;
  }
  return gl::GLenum::GL_NONE;
}
inline gl::GLenum toGlType(ImageFormatEnum format)
{
  switch (format)
  {
  case ImageFormatEnum::eRg32f:
  case ImageFormatEnum::eRgba32f:
  case ImageFormatEnum::eFloat:
    return gl::GLenum::GL_FLOAT;
  case ImageFormatEnum::eSnorm16:
    return gl::GLenum::GL_SHORT;
  case ImageFormatEnum::eUnorm16:
    return gl::GLenum::GL_UNSIGNED_SHORT;
  case ImageFormatEnum::eUnorm8:
  case ImageFormatEnum::eRgba8:
  case ImageFormatEnum::eSrgb8Alpha8:
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

inline auto toGl(GfxImage::ComponentValue comp)
{
  switch (comp)
  {
  case GfxImage::ComponentValue::eRed:
    return gl::GL_RED;
  case GfxImage::ComponentValue::eGreen:
    return gl::GL_GREEN;
  case GfxImage::ComponentValue::eBlue:
    return gl::GL_BLUE;
  case GfxImage::ComponentValue::eAlpha:
    return gl::GL_ALPHA;
  case GfxImage::ComponentValue::eZero:
    return gl::GL_ZERO;
  default:
    return gl::GL_ONE;
  }
}

inline auto toGl(GfxAccess access)
{
  switch (access)
  {
  case GfxAccess::eReadOnly:
    return gl::GL_READ_ONLY;
  case GfxAccess::eWriteOnly:
    return gl::GL_WRITE_ONLY;
  }
  return gl::GL_READ_WRITE;
}
} // namespace terra