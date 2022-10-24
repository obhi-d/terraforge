#pragma once
#include <map>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "Common.h"
#include "CurveData.h"
#include "DataSource.h"
#include "Dependency.h"
#include "GpuBuffer.h"
#include "Image.h"
#include "ComputeDevice.h"
#include "Serializer.h"

namespace terra
{
class Terra;

using Parameter = std::variant<ScalarValue, Source>;
using TaskKey   = uint64_t;
enum class Result
{
  eFinished,
  eContinue,
  eAbort
};

struct ParameterMeta
{
  std::string        id;
  int32              uboOffset       = -1;
  int32              descriptorIndex = -1;
  DataFormat         format;
  DrawHint           drawHint = DrawHint::eDefault;
  
  std::u8string_view name;
  std::u8string_view help;
  std::u8string_view tooltip;

  int16_t optionIndex = {};
  int16_t optionCount = 1;

  enum ValueType
  {
    eDefault = 0,
    eMin,
    eMax,
    eStep,
    eCount
  };

  DataValue                             values[ValueType::eCount] = {};
  std::unique_ptr<std::u8string_view[]> enumValues                = {};

  ParameterMeta()                                         = default;
  ParameterMeta(ParameterMeta const&)                     = delete;
  ParameterMeta(ParameterMeta&&) noexcept                 = default;
  ParameterMeta& operator=(ParameterMeta const&) noexcept = delete;
  ParameterMeta& operator=(ParameterMeta&&) noexcept      = default;

  inline bool isValid() const
  {
    return format.type != DataType::eInvalid;
  }

  bool canBeSource() const;
  bool canBeScalar() const;

  ScalarValue getDefault() const;

  bool affectsOptions() const;
  bool modifyOptions(Pipeline& pipe, Parameter const&, Options&) const;
  void setTypeFromString(std::string_view);
  void setValueFromString(ValueType, std::string_view);
};

struct AttributeMeta
{
  DataType scalarType;
  DrawHint drawHint = DrawHint::eDefault;

  std::u8string_view name;
  std::u8string_view help;
  std::u8string_view tooltip;

  DataValue                             defaultValue = {};
  std::unique_ptr<std::u8string_view[]> enumValues   = {};
};

class Node;
class NodeMeta
{

public:
  struct ShaderContent
  {
    std::string function;
    std::string extensions;
    std::string shaderContent;
  };

  std::string                    id;
  std::u8string                  icon;
  std::u8string_view             name;
  std::u8string_view             category;
  std::u8string_view             tooltip;
  std::u8string_view             help;
  std::string                    style;
  std::vector<ParameterMeta>     parameterDef;
  int32_t                        nbDescriptors          = 0;
  int32_t                        outputDescriptorIdx    = 0;
  int32_t                        constantsDescriptorIdx = 0;
  uint32_t                       outputUpscale          = 1; // multiplier
  uint32_t                       outputDownscale        = 1; // divisor for reduction algo
  int32_t                        uboSize                = 0;
  uint32_t                       outputSemantic         = 0;
  ImageFormat                    imageFormat            = ImageFormat::eFloat;
  DataFormat                     format;
  GfxDescriptorSetLayout::handle descriptorSetLayout;
  std::vector<std::string>       options;

  bool hasTextureOutput = false;
  bool hasUniforms      = false;
  bool cacheResults     = false;
  bool modifiesInput    = false;
  bool modifiesOutput   = true;

  using Ensure         = Result (*)(Node&, Pipeline&);
  using Run            = Result (*)(Node&, Pipeline&);
  using FillDescriptor = void (*)(Node&, Pipeline const& pipeline, GfxDescriptorSet::rhandle& rh, std::byte* data);
  using BuildGLSL      = void (*)(NodeMeta&, ShaderContent const&);
  using IsEnabled      = bool (*)(Node const&, Pipeline const&);
  // override run
  Ensure         ensure         = nullptr;
  Run            run            = nullptr;
  FillDescriptor fillDescriptor = nullptr;
  BuildGLSL      buildGLSL      = nullptr;
  IsEnabled      isEnabled      = nullptr;

  // attributes
  bool attribTileConstrained = false;
  bool attribIteration       = false;
  //

  uint32_t findParam(std::string_view name) const
  {
    for (uint32_t i = 0; i < (uint32_t)parameterDef.size(); ++i)
      if (parameterDef[i].id == name)
        return i;
    return ~0u;
  }

  GfxProgram::handle getShaderGLSL(Options optionBitSet) const;

  NodeMeta()                                         = default;
  NodeMeta(NodeMeta const&)                     = delete;
  NodeMeta(NodeMeta&&) noexcept                 = default;
  NodeMeta& operator=(NodeMeta const&) noexcept = delete;
  NodeMeta& operator=(NodeMeta&&) noexcept      = default;

  ~NodeMeta();
  void destroy();

  void buildShaderGLSL(ShaderContent const&);

private:
  std::shared_ptr<ShaderBuilder>                          shaderBuilder;
  mutable std::unordered_map<Options, GfxProgram::handle> shaders;
};

class Node : public DataSource
{
public:
  Node()                                = default;
  Node(Node&&)                          = default;
  Node(Node const&)                     = delete;
  Node& operator=(Node&&) noexcept      = default;
  Node& operator=(Node const&) noexcept = delete;
  Node(NodeMeta const&);
  ~Node();

  void markValueChanged();

  uint32_t getNumParams() const
  {
    return (uint32_t)parameters.size();
  }
  Type getType() const final
  {
    return Type::eNode;
  }
  DataFormat getFormat() const final
  {
    return meta->format;
  }
  auto& getMeta() const
  {
    return *meta;
  }
  Parameter const& getValue(uint32_t i) const
  {
    return parameters[i];
  }
  bool hasTextureOutput() const
  {
    return meta->hasTextureOutput;
  }

  template <typename L>
  int32_t forEachSource(L&& lambda)
  {
    int32_t hasEdges = 0;
    for (uint32_t i = 0; i < (uint32_t)parameters.size(); ++i)
    {
      if (std::holds_alternative<Source>(parameters[i]))
      {
        auto node = std::get<Source>(parameters[i]);
        if (node.source && isValid(node.source))
        {
          hasEdges++;
          if (!lambda(i, node.source))
            return -1;
        }
      }
    }
    return hasEdges;
  }

  int32_t getOutputId(TaskKey task) const
  {
    auto it = tasks.find(task);
    if (it != tasks.end())
      return it->second.outputId;
    return -1;
  }

  bool setValue(uint32_t i, ScalarValue value); // retunrs old source of data
  bool setValue(uint32_t i, Source value);      // retunrs old source of data
  void resetValue(uint32_t i);
  void setValueModified(uint32_t i);
  void deleteTaskData(uint32_t taskId);
  // int32_t incomingEdges() const;
  dshandle clone(uint32_t iteration);

  std::u8string_view getName() const
  {
    return name;
  }

  auto const& param(uint32 i) const
  {
    return parameters[i];
  }

  auto& param(uint32 i)
  {
    return parameters[i];
  }

  auto const& paramMeta(uint32 i) const
  {
    return meta->parameterDef[i];
  }

  void accept(dshandle source, Event) final;

  bool isEnabled(Pipeline const&) const final;

  bool ensure(Pipeline&) final;

  // GPU thread functions
  Result run(Pipeline&);
  void   fillDescriptor(Pipeline const&, GfxDescriptorSet::rhandle&, std::byte*) final;

private:
  struct TaskData
  {
    int32_t                  outputId = -1;
    GfxDescriptorSet::handle descriptorSet;
    uint32_t                 outputX = 0;
    uint32_t                 outputY = 0;
    EnvParams                params;
    GfxProgram::handle       shader;
    int32_t                  iteration    = -1;
    bool                     valueChanged = true;
    std::vector<Parameter>   parameters;
  };

  std::u8string                         name;
  std::unordered_map<TaskKey, TaskData> tasks;
  NodeMeta const*                       meta = nullptr;
  std::vector<Parameter>                parameters;
  
  // predefined attribute values
  uvec2   tileConstraintOffset = {0, 0};
  uvec2   tileConstraintSize   = {1, 1};
  float   defaultValue         = 1.0f;
  int32_t maxIteration         = 1;
  //

  bool alreadyComputed(Pipeline const&);

  void     sourceDeleted(dshandle src);
  bool     fromDataStreamImpl(const std::vector<uint8_t>& dataStream, size_t& serialIdx) final;
  void     toDataStreamImpl(std::vector<uint8_t>& dataStream) const final;
  exchange setParamSourceImpl(uint32_t paramIdx, Source) final;
};
} // namespace terra