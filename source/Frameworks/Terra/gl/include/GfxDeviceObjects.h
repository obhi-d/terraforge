#pragma once
#include "GlGfx.h"
#include "Table.h"
#include <acl/dynamic_array.hpp>
#include <unordered_map>

namespace terra
{
struct GfxBufferGl : GfxBuffer
{
  gl::GLuint       glhandle = {};
  gl::GLenum       target;
  GfxStorageClass  storage;
  GfxBuffer::Usage usage;
  uint32_t         size  = {};
  gl::GLuint       gltbo = {};
  gl::GLuint64     hdev  = 0;
};

struct GfxImageGl : GfxImage2D
{
  gl::GLuint                                   glhandle = {};
  GfxStorageClass                              storage;
  uint32_t                                     width  = {};
  uint32_t                                     height = {};
  ImageFormat                                  format;
  gl::GLuint64                                 hdev = 0;
  std::unordered_map<gl::GLuint, gl::GLuint64> hdevTexSampler;
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

struct GfxBindlessLayoutGl : GfxBindlessLayout
{

  acl::dynamic_array<Entry> entries;
};

struct GfxBindlessDescriptorGl : GfxBindlessDescriptor
{
  GfxBindlessLayout::handle layout;
  gl::GLuint                glhandle     = 0;
  uint32_t                  bufferOffset = 0;
  uint32_t                  bufferSize   = 0;
};

struct GfxResources
{
  std::vector<std::uint8_t> uboData;

  table<GfxBufferGl>  buffers;
  table<GfxImageGl>   images;
  table<GfxSamplerGl> samplers;

  table<GfxDescriptorSetLayoutGl> descriptorSetLayouts;
  table<GfxDescriptorSetGl>       descriptorSets;
  table<GfxFenceGl>               fences;

  table<GfxProgramGl>                  programs;
  table<GfxMeshLayoutGl>               meshes;
  table<GfxBindlessLayoutGl>           bindlessLayout;
  std::vector<GfxBindlessDescriptorGl> bindlessDescriptors;

  using MeshMap = std::unordered_map<GfxMesh::Layout, GfxMesh::handle, GfxMesh::LayoutHash>;
  MeshMap meshMap;
};

} // namespace terra