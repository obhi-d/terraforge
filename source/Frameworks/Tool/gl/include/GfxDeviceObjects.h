#pragma once
#include "GlGfx.h"
#include "Table.h"

namespace terra
{
struct GfxBufferGl
{
  gl::GLuint       glhandle;
  GfxStorageClass  storage;
  GfxBuffer::Usage usage;
  uint32_t         size;
};

struct GfxImageGl
{
  gl::GLuint      glhandle;
  GfxStorageClass storage;
  uint32_t        width;
  uint32_t        height;
  ImageFormat     format;
};

struct GfxSamplerGl
{
  gl::GLuint glhandle;
};

struct GfxDescriptorSetLayoutGl
{
  std::unique_ptr<GfxDescriptorSetLayout::Descriptor[]> descriptors;
  uint32_t                                              descriptorCount = 0;
};

struct GfxDescriptorSetGl
{
  GfxDescriptorSetLayout::handle               layout;
  std::unique_ptr<GfxDescriptorSet::rhandle[]> values;
};

struct GfxFenceGl
{
  gl::GLsync sync;
};

struct GfxProgramGl
{
  gl::GLuint glhandle;
  gl::GLuint shaders[ShaderTypeCount] = {};

  // for a descriptor set layout, store locations
  std::vector<gl::GLuint> locations;
};

struct GfxMeshLayoutGl
{
  GfxMesh::Layout const* desc         = nullptr;
  uint32           usageCounter = 0;
  gl::GLuint       glhandle;
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