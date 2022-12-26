#pragma once
#include "GlGfx.h"
#include "Table.h"
#include <acl/dynamic_array.hpp>
#include <unordered_map>
#include <unordered_set>

namespace terra
{

enum class BindlessHandleType : uint8_t
{
  eNone,
  eTexture,
  eImage,
};

struct BindlessHandleGl
{
  using handle                     = terra::handle<BindlessHandleGl>;
  gl::GLuint64       hdev          = 0;
  uint32_t           residentFrame = 0;
  BindlessHandleType type          = BindlessHandleType::eNone;
  GfxAccess          access        = GfxAccess::eReadOnly;
  bool               active        = false;
  bool               resident      = false;
};

struct GfxBufferGl : GfxBuffer
{
  gl::GLuint               glhandle = {};
  gl::GLenum               target;
  GfxStorageClass          storage;
  GfxBuffer::Usage         usage;
  uint32_t                 size  = {};
  gl::GLuint               gltbo = {};
  BindlessHandleGl::handle hdev  = 0;
};

struct GfxImageGl : GfxImage
{
  using BindlessSamplerMap = std::unordered_map<GfxSampler::handle, BindlessHandleGl::handle, HandleHash<GfxSampler>>;

  gl::GLuint               glhandle = {};
  GfxStorageClass          storage;
  uint32_t                 width  = {};
  uint32_t                 height = {};
  uint32_t                 layers = 1;
  ImageFormatEnum          format;
  BindlessHandleGl::handle hdev = 0;
  BindlessHandleGl::handle himg = 0;
  uint32_t                 ref  = 0;
  gl::GLuint               fbo  = 0; // any related fbo

  BindlessSamplerMap hsamplerMap;
};

struct GfxPassGl : GfxPass
{
  std::array<GfxImage::handle, 8> images;
  std::array<vec4, 8>             imageClearColors = {};
  std::array<bool, 8>             imageClears      = {};

  GfxImage::handle depth;
  float            depthClearValue = 0.f;

  gl::GLuint glhandle   = 0;
  uint32_t   nbImages   = 0;
  bool       depthClear = false;
};

struct GfxSamplerGl : GfxSampler
{
  gl::GLuint glhandle = {};
};

struct GfxDescriptorSetLayoutGl : GfxDescriptorSetLayout
{
  std::unique_ptr<GfxDescriptorSetLayout::Descriptor[]> descriptors;
  uint32_t                                              descriptorCount = 0;
};

struct GfxDescriptorSetGl : GfxDescriptorSet
{
  GfxDescriptorSetLayout::handle               layout;
  std::unique_ptr<GfxDescriptorSet::rhandle[]> values;
};

struct GfxFenceGl : GfxFence
{
  gl::GLsync sync = {};
};

struct GfxProgramGl : GfxProgram
{
  gl::GLuint glhandle                 = {};
  gl::GLuint shaders[ShaderTypeCount] = {};
};

struct GfxMeshLayoutGl : GfxMesh
{
  GfxMesh::Layout const* desc = nullptr;
  GfxBuffer::handle      vertexBuffers[4];
  GfxBuffer::handle      elementBuffer          = 0;
  uint32_t               vertexBufferOffsets[4] = {};
  uint32                 usageCounter           = 0;
  gl::GLuint             glhandle;
};

struct GfxBindlessLayoutGl : GfxParamLayout
{
  acl::dynamic_array<Entry> entries;
};

struct GfxFramebufferGl
{
  gl::GLuint glhandle           = 0;
  uint8_t    activeAttachments  = 0;
  bool       hasDepthAttachment = false;
};

struct GfxResources
{
  table<GfxBufferGl>  buffers;
  table<GfxImageGl>   images;
  table<GfxSamplerGl> samplers;
  table<GfxPassGl>    passes;

  table<GfxDescriptorSetLayoutGl> descriptorSetLayouts;
  table<GfxDescriptorSetGl>       descriptorSets;
  table<GfxFenceGl>               fences;

  table<GfxProgramGl>        programs;
  table<GfxMeshLayoutGl>     meshes;
  table<GfxBindlessLayoutGl> bindlessLayout;
  table<BindlessHandleGl>    bindlessHandles;

  std::vector<BindlessHandleGl::handle> activeResidents;

  using MeshMap = std::unordered_map<GfxMesh::Layout, GfxMesh::handle, GfxMesh::LayoutHash>;
  MeshMap meshMap;
};

} // namespace terra