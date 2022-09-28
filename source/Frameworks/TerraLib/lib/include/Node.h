#pragma once
#include <string>
#include <variant>
#include <vector>

#include "Common.h"
#include "CurveData.h"
#include "DataSource.h"
#include "ImageData.h"

namespace terra
{
enum class ParameterType
{
  eInt,
  eFloat,
  eInt2,
  eFloat2,
  eBool,
  eImage,
  eDataSource,
  eCurveData,
  eInvalid
};

enum class DrawHint
{
  eDefault,  // newline
  eSameline, // same line as the previous param
};

struct ParameterMeta
{
  std::string   name;
  uint32        uboOffset   = 0;
  uint32        availOffset = 0;
  uint32        binding     = 0;
  ParameterType type        = ParameterType::eInvalid;
  DrawHint      drawHint    = DrawHint::eDefault;
  union
  {
    float fmax;
    int   imax = 0;
  };
  union
  {
    float fmin;
    int   imin = 0;
  };
  union
  {
    float fdefault;
    int   idefault = 0;
  };

  enum class ValueType
  {
    eDefault,
    eMin,
    eMax
  };

  inline bool isValid() const
  {
    return type != ParameterType::eInvalid;
  }

  void setTypeFromString(std::string_view);
  void setValueFromString(ValueType, std::string_view);
};

struct NodeMeta
{
  std::string                function;
  std::vector<ParameterMeta> parameterDef;
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