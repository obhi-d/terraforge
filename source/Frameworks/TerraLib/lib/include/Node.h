#pragma once
#include <string>
#include <variant>
#include <vector>

#include "Common.h"
#include "CurveData.h"
#include "DataSource.h"
#include "ImageData.h"
#include "RenderDevice.h"

namespace terra
{
class Terra;
enum class ParameterType
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

enum class DrawHint
{
  eDefault,  // newline
  eSameline, // same line as the previous param
};

union ParamValue
{
  float fval;
  int   ival = 0;

  ParamValue(float val) : fval(val) {}
  ParamValue(int val) : ival(val) {}
};

struct EnvParams
{
  float  frequency;
  float  wavelength;
  float2 start;

  float2 size;
  float2 center;

  float2 gridSize;
  float2 recipSize;

  float2 recipGridSize;
  float2 halfRecipGridSize;

  int2 bufferSize;
  int2 startCoord;

  int seed;
  int reserved;
};

struct ParameterMeta
{
  std::string   name;
  uint32        uboOffset   = 0;
  uint32        availOffset = 0;
  uint32        binding     = 0;
  ParameterType type        = ParameterType::eInvalid;
  DrawHint      drawHint    = DrawHint::eDefault;
  std::string   sampler;

  enum ValueType
  {
    eDefault = 0,
    eMin,
    eMax,
    eStep,
    eCount
  };

  ParamValue values[ValueType::eCount];

  inline bool isValid() const
  {
    return type != ParameterType::eInvalid;
  }

  void setTypeFromString(std::string_view);
  void setValueFromString(ValueType, std::string_view);
};

struct NodeMeta
{
  std::u8string name;
  std::u8string category;
  std::u8string brief;
  std::u8string help;

  std::string function;

  std::string                extensions;
  std::string                shaderContent;
  uint32_t                   paramSize = 0;
  std::vector<ParameterMeta> parameterDef;

  GfxCompute::handle shader;

  bool buildShader(Terra&, RenderDevice&);

  static std::string writeTextureSampler(std::string_view);
  static std::string writeDataSampler(std::string_view);
  static std::string writeCurveSampler(std::string_view);
};

using Parameter = std::variant<int, float, int2, float2, bool, ImageData, DataSource, CurveDataPtr>;
class Node
{
public:
private:
  NodeMeta&              desc;
  std::vector<Parameter> parameters;
};

using NodePtr = std::shared_ptr<Node>;
} // namespace terra