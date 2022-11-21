#pragma once
#include "GlGfx.h"
#include "Table.h"

namespace terra
{
struct GfxBufferGl : GfxBuffer
{
  gl::GLuint       glhandle;
  gl::GLenum       target;
  GfxStorageClass  storage;
  GfxBuffer::Usage usage;
  uint32_t         size;
};

struct GfxImageGl : GfxImage2D
{
  gl::GLuint      glhandle;
  GfxStorageClass storage;
  uint32_t        width;
  uint32_t        height;
  ImageFormat     format;
};

struct GfxSamplerGl : GfxSampler
{
  gl::GLuint glhandle;
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
  gl::GLsync sync;
};

struct GfxProgramGl : GfxProgram
{
  gl::GLuint glhandle;
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

struct GfxResources
{
  table<GfxBufferGl>  buffers;
  table<GfxImageGl>   images;
  table<GfxSamplerGl> samplers;

  table<GfxDescriptorSetLayoutGl> descriptorSetLayouts;
  table<GfxDescriptorSetGl>       descriptorSets;
  table<GfxFenceGl>               fences;

  table<GfxProgramGl>    programs;
  table<GfxMeshLayoutGl> meshes;

  using MeshMap = std::unordered_map<GfxMesh::Layout, GfxMesh::handle, GfxMesh::LayoutHash>;
  MeshMap meshMap;
};

} // namespace terra