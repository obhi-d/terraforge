#include "Pipeline.h"
#include "Terra.h"

namespace terra
{

Pipeline::Image::~Image() 
{
  terra::get().getDevice().destroy(image);
}

void Pipeline::prepare(DispatchTask&& task)
{
  this->task = task;
  ubo.setDesc(GfxBuffer::Usage::fUniform, GfxStorageClass::eDynamicDeviceReadonly);
}

int32_t Pipeline::compute(hnode index)
{
  prepareNodes(index);
  nodes.push_back(index);
  return (int32_t)nodes.size() - 1;
}

int32_t Pipeline::computePreview(hnode index)
{
  auto& main = Terra::get();
  auto  meta = main.getNodeMeta("Utility.getMinMax");

  auto minMaxId = main.createNode(*meta);
  if (!meta)
    return ~0u;
  nodesToDelete.push_back(minMaxId);
  auto& minmax = main.getNode(minMaxId);

  minmax.enqueue(task.taskID, 0, *this);
  minmax.setValue(0, DataSource(index));

  meta         = main.getNodeMeta("Modifiers.toRGBA");
  if (!meta)
    return ~0u;
  auto rgbaId = main.createNode(*meta);
  nodesToDelete.push_back(rgbaId);
  auto& rgba = main.getNode(rgbaId);

  rgba.enqueue(task.taskID, 0, *this);
  rgba.setValue(meta->findParam("MinMax"), DataSource(minMaxId));
 
  prepareNodes(rgbaId);
  nodes.push_back(rgbaId);

  return (int32_t)nodes.size() - 1;
}

void Pipeline::allocateResources()
{
  uint32_t phyId  = 0;
  uint32_t nbBuff = (uint32_t)buffers.size();
  for (uint32_t i = 0; i < nbBuff; ++i)
    buffers[i].phyId = 0;
  for (uint32_t i = 0; i < nbBuff; ++i)
  {
    if (buffers[i].phyId)
      continue;
    buffers[i].phyId = ++phyId;
    for (uint32_t j = i + 1; j < nbBuff; ++j)
    {
      // x1 <= y2 && y1 <= x2
      if (buffers[j].phyId || (buffers[i].read <= buffers[j].write && buffers[i].write <= buffers[j].read))
        continue;
      buffers[j].phyId = phyId;
    }
  }
  gpuBuffers.resize(phyId);
  for (uint32_t i = 0; i < nbBuff; ++i)
    gpuBuffers[buffers[i].phyId].setSize(buffers[i].size);

  for (auto& gpu : gpuBuffers)
  {
    gpu.setDesc(GfxBuffer::Usage::fStorage, GfxStorageClass::eDynamicDeviceAccess);
    gpu.ensure();
  }

  auto& rd = Terra::get().getDevice();
  for (auto& img : images)
    img.image = rd.createImage(GfxStorageClass::eDeviceAccess, img.width, img.height, img.format);
}

void Pipeline::prepareNodes(hnode index)
{
  nodes.emplace_back(index);
  auto& main = Terra::get();
  auto& node = main.getNode(index);
  if (node.isReadyToExecute(task.taskID))
    return;
  auto& meta = node.getMeta();
  if (meta.iteration == 1)
    node.enqueue(task.taskID, 0, *this);
  else
  {
    auto     totalBufferSize = (task.params.size[0] + 2) * (task.params.size[1] + 2);
    auto     source          = index;
    uint32_t sourceIdx       = meta.findParam("Source");
    uint32_t iterEnd         = meta.iteration;
    if (meta.iteration == std::numeric_limits<uint32_t>::max())
    {
      uint32_t downscale = meta.outputDownscale;
      if (downscale <= 1)
        downscale = 2; // downscale so that we do not loop forever
      uint32_t size = totalBufferSize;
      iterEnd       = 0;
      while (size > 0)
      {
        iterEnd++;
        size /= downscale;
      }
    }
    // O -> N (O currently links to N)
    // What we are doing is inserting a number of intermediate nodes
    // O->Nn...->N2->N1->N 
    if (iterEnd <= 1)
      node.enqueue(task.taskID, 0, *this);
    else
    {
      auto source = std::get<DataSource>(node.getValue(sourceIdx));      
      for (uint32_t i = 0; i < iterEnd - 1; ++i)
      {
        auto newNode = node.clone(i);
        nodesToDelete.push_back(newNode);
        auto& iternode = main.getNode(newNode);
        iternode.enqueue(task.taskID, i, *this);
        iternode.setValue(sourceIdx, source);
        source.node = newNode;        
      }
      node.setValue(sourceIdx, source);
    }
  }
  node.forEachSource(
    [this](auto innode)
    {
      prepareNodes(innode);
    });
}

void Pipeline::gatherLeafs(hnode nodeid, std::unordered_map<int32_t, int>& edgeMap, std::vector<hnode>& leafs) 
{
  if (edgeMap.find(nodeid) != edgeMap.end())
    return;
  auto& node               = Terra::get().getNode(nodeid);
  int   edges = node.forEachSource(
    [&](hnode n)
    {
      gatherLeafs(n, edgeMap, leafs);
    });
  edgeMap[(int32_t)nodeid] = edges;
  if (!edges)
    leafs.push_back(nodeid);
}

void Pipeline::sortNodes() 
{
  auto&                            main = Terra::get();
  std::vector<hnode>             leafs;
  std::unordered_map<int32_t, int> incoming;
  for (auto& n : nodes)
    gatherLeafs(n, incoming, leafs);

  actors.clear();
  while (!leafs.empty())
  {
    auto nodeId = leafs.back();
    leafs.pop_back();
    actors.push_back(nodeId);
    auto& node = main.getNode(nodeId);
    node.forEachDependent([&](hnode nodeId) 
      {
        int  edges = 0;
        auto it    = incoming.find(nodeId);
        if (it == incoming.end())
        {
          auto& node                = main.getNode(nodeId);
          edges                     = node.incomingEdges() - 1;
          incoming[(int32_t)nodeId] = edges;
        }
        else
          edges = --it->second;
        if (!edges)
        {
          leafs.push_back(nodeId);
        }
      });
  }

  for (int32_t order = 0; order < (int32_t)actors.size(); ++order)
  {
    main.getNode(actors[order]).setOrder(task.taskID, order);
  }
}

void Pipeline::determineBufferLifetimes()
{
  auto& main = Terra::get();
  for (int32_t i = 0; i < (int32_t)actors.size(); ++i)
  {
    auto& nodeId = actors[i];
    auto& node = main.getNode(nodeId);
    if (!node.hasTextureOutput())
    {
      auto id = main.getNode(nodeId).getOutputId(task.taskID);
      if (id >= 0)
        buffers[id].write = i;
    }
    node.forEachSource(
      [&](hnode innode)
      {
        auto& src = main.getNode(innode);
        auto  srcid  = src.getOutputId(task.taskID);
        if (srcid >= 0)
          buffers[srcid].read = std::max(buffers[srcid].read, i);
      });
  }
}

int32_t Pipeline::declBuffer(uint32_t size) 
{
  buffers.emplace_back();
  buffers.back().size = size;
  return (int32_t)buffers.size() - 1;
}

int32_t Pipeline::declImage(uint32_t width, uint32_t height, ImageFormat fmt)
{
  images.emplace_back();
  images.back().width = width;
  images.back().height = height;
  images.back().format = fmt;
  return (int32_t)images.size() - 1;
}

void Pipeline::execute() 
{
  auto& main = Terra::get();
  sortNodes();
  determineBufferLifetimes();
  allocateResources();
  ubo.ensure();
  auto buff = ubo.map(0, sizeof(EnvParams));
  std::memcpy(buff, &task.params, sizeof(EnvParams));
  ubo.unmap();
  for (auto actor : actors)
  {
    main.getNode(actor).run(task.taskID, *this);
  }
  sync = main.getDevice().createFence();
}

void Pipeline::wait() 
{
  auto& main = Terra::get();
  main.getDevice().syncFence(sync);
  sync = {}; 
  for (auto actor : actors)
  {
    main.getNode(actor).deleteTaskData(task.taskID);
  }
}

void Pipeline::cleanup() 
{
  for (auto n : nodesToDelete)
    terra::get().destroy(n);
  buffers.clear();
  images.clear();
  gpuBuffers.clear();
}

GfxBuffer::handle Pipeline::readOutputBuffer(int32_t nodeIdx) const
{
  return getOutputBuffer(nodes[nodeIdx]);
}

GfxImage2D::handle Pipeline::readOutputImage(int32_t nodeIdx) const
{
  return getOutputImage(nodes[nodeIdx]);
}

void Pipeline::readOutputBufferContent(int32_t nodeIdx, std::span<std::byte> out) const
{
  get().getDevice().readBuffer(readOutputBuffer(nodeIdx), 0, out);
}

void Pipeline::readOutputImageContent(int32_t nodeIdx, std::span<std::byte> out) const
{
  get().getDevice().readImage(readOutputImage(nodeIdx), out);
}

GfxBuffer::handle  Pipeline::getOutputBuffer(hnode nodeIdx) const 
{
  auto& node = terra::get().getNode(nodeIdx);
  return gpuBuffers[(uint32_t)buffers[(uint32_t)node.getOutputId(task.taskID)].phyId].get();
}

GfxImage2D::handle Pipeline::getOutputImage(hnode nodeIdx) const 
{
  auto& node = terra::get().getNode(nodeIdx);
  return images[(uint32_t)node.getOutputId(task.taskID)].image;
}

} // namespace terra