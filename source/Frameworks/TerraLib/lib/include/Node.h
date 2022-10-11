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
#include "ImageData.h"
#include "RenderDevice.h"
#include "Serializer.h"

namespace terra
{
class Terra;
enum class DataType
{
  eInt2,
  eFloat2,
  eInt,
  eFloat,
  eImage,
  eDataSource,
  eCurveData,
  eBool,
  eInvalid
};

struct DataFormat
{
  DataType    type                                          = DataType::eInvalid;
  DataType    scalarSubType                                 = DataType::eFloat;
  uint32_t    semantic                                      = 0; // Strict rule for matching input
  inline auto operator<=>(const DataFormat&) const noexcept = default;
};

DataType    stringToType(std::string_view);
std::string_view typeToString(DataType);

enum class DrawHint
{
  eDefault,  // newline
  eSameline, // same line as the previous param
  eHidden
};

union DataValue
{
  float fval;
  int   ival = 0;

  DataValue() = default;
  DataValue(float val) : fval(val) {}
  DataValue(int val) : ival(val) {}
};

struct EnvParams
{
  float frequency;
  float wavelength;

  vec2 gridSize;
  vec2 recipGridSize;

  uint2 size;
  uint2 startCoord;

  uint32_t seed;
  uint32_t bufferArraySize;
};

using Parameter = std::variant<std::monostate, ScalarValue, DataSource, ImageSource, CurveDataPtr>;

struct ParameterMeta
{
  std::string   name;
  int32         uboOffset       = -1;
  int32         descriptorIndex = -1;
  DataFormat         format;
  DrawHint           drawHint = DrawHint::eDefault;
  std::string        sampler;
  std::u8string_view help;
  std::u8string_view tooltip;

  std::array<int, 2> optionIndex = {};

  enum ValueType
  {
    eDefault = 0,
    eMin,
    eMax,
    eStep,
    eCount
  };

  DataValue values[ValueType::eCount] = {};
  

  inline bool isValid() const
  {
    return format.type != DataType::eInvalid;
  }

  Parameter getDefault() const;

  bool affectsOptions() const;
  void modifyOptions(Parameter const&, Options&) const;
  void setTypeFromString(std::string_view);
  void setValueFromString(ValueType, std::string_view);
};

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
  uint32_t                       iteration              = 1;
  int32_t                        uboSize                = 0;
  uint32_t                       outputSemantic         = 0;
  ImageFormat                    imageFormat            = ImageFormat::eFloat;
  DataFormat                     format;
  GfxDescriptorSetLayout::handle descriptorSetLayout;
  std::vector<std::string>       options;
  bool                           hasTextureOutput = false;
  bool                           hasUniforms      = false;

  uint32_t findParam(std::string_view name) const
  {
    for (uint32_t i = 0; i < (uint32_t)parameterDef.size(); ++i)
      if (parameterDef[i].name == name)
        return i;
    return ~0u;
  }

  GfxProgram::handle getShaderGLSL(Options optionBitSet) const;

  ~NodeMeta();
  void destroy();

  void buildShaderGLSL(ShaderContent const&);

private:
  std::shared_ptr<ShaderBuilder>                          shaderBuilder;
  mutable std::unordered_map<Options, GfxProgram::handle> shaders;

  static std::string writeTextureSamplerGLSL(std::string_view);
  static std::string writeDataSamplerGLSL(RenderDevice::Caps const& caps, std::string_view);
  static std::string writeCurveSamplerGLSL(RenderDevice::Caps const&, std::string_view);
  static std::string writeImageStoreGLSL(std::string_view);
  static std::string writeBufferStoreGLSL(std::string_view);
};

class Node : public Dependency
{
public:
  Node() = default;
  Node(NodeMeta const&);
  ~Node();

  void markValueChanged()
  {
    valueChanged = true;
  }
  void markOptionChanged()
  {
    optionChanged = true;
  }
  uint32_t getNumParams() const
  {
    return (uint32_t)parameters.size();
  }
  DataFormat const& getFormat() const
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
  void setId(uint32_t id)
  {
    this->id = id;
  }
  auto getId() const
  {
    return id;
  }
  template <typename L>
  int32_t forEachSource(L&& lambda)
  {
    int32_t hasEdges = 0;
    for (uint32_t i = 0; i < (uint32_t)parameters.size(); ++i)
    {
      if (meta->parameterDef[i].format.type == DataType::eDataSource)
      {
        auto node = std::get<DataSource>(parameters[i]).node;
        if (node && isValid(node))
        {
          hasEdges++;
          lambda(node);
        }
      }
      else if (meta->parameterDef[i].format.type == DataType::eImage)
      {
        auto& img = std::get<ImageSource>(parameters[i]);
        if (std::holds_alternative<hnode>(img.source))
        {
          auto node = std::get<hnode>(img.source);
          if (node && isValid(node))
          {
            hasEdges++;
            lambda(node);
          }
        }
 
      }
    }
    return hasEdges;
  }
  void setOrder(uint32_t taskId, int32_t order)
  {
    tasks[taskId].execOrder = order;
  }

  int32_t getOutputId(uint32_t task) const
  {
    return tasks[task].outputId;
  }

  void sourceDeleted(hnode src);

  bool        isInputCompatible(uint32_t i, DataFormat const&);
  void        setValue(uint32_t i, Parameter&& value);
  void        setValueModified(uint32_t i);
  void        prepare(uint32_t token);
  bool        isReadyToExecute(uint32_t taskId);
  void        deleteTaskData(uint32_t taskId);
  void        enqueue(uint32_t taskId, uint32_t iteration, Pipeline&);
  void        run(uint32_t taskId, Pipeline&);
  void        finish(uint32_t taskId, Pipeline&);
  int32_t     incomingEdges() const;
  hnode       clone(uint32_t iteration);
  static bool isValid(hnode);

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

private:
  struct TaskData
  {
    bool                     ready     = false;
    int32_t                  execOrder = 0;
    int32_t                  outputId  = -1;
    GfxDescriptorSet::handle descriptorSet;
    uint32_t                 outputX = 0;
    uint32_t                 outputY = 0;
    EnvParams                params;
  };

  std::u8string          name;
  std::vector<TaskData>  tasks;
  hnode                  id;
  NodeMeta const*        meta = nullptr;
  std::vector<Parameter> parameters;
  GfxProgram::handle     shader;
  uint32_t               prepareToken  = std::numeric_limits<uint32_t>::max();
  bool                   valueChanged  = true;
  bool                   optionChanged = true;

  bool fromDataStream(const std::vector<uint8_t>& dataStream, size_t& serialIdx);
  void toDataStream(std::vector<uint8_t>& dataStream) const;
};
} // namespace terra