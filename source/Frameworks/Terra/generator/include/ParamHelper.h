
#pragma once

#include <variant>
#include <vector>

#include "Common.h"
#include "CurveData.h"
#include "DataSource.h"
#include "Dependency.h"
#include "GpuBuffer.h"
#include "Image.h"

namespace terra
{

struct ParamHelper
{
  using type = std::variant<bool, float, int, uint32_t, vec2, ivec2, uvec2, vec3, glm::ivec3, glm::uvec3, vec4,
                            glm::ivec4, glm::uvec4, ArrayFloatRef, ArrayIntRef, ArrayUintRef, Source, BufferRef,
                            FixedString, Switch, std::monostate>;

  enum Index
  {
    eBool,
    eFloat,
    eInt,
    eUint,
    eFloat2,
    eInt2,
    eUint2,
    eFloat3,
    eInt3,
    eUint3,
    eFloat4,
    eInt4,
    eUint4,
    eArrayFloat,
    eArrayInt,
    eArrayUint,
    eSource,
    eBuffer,
    eString,
    eSwitch,
    eInvalid
  };

  static uint16_t scalarSize(DataTypeEnum ty)
  {
    switch (ty)
    {
    case DataTypeEnum::eUint:
      return sizeof(uint32_t);
    case DataTypeEnum::eUint2:
      return sizeof(uint32_t) * 2;
    case DataTypeEnum::eInt:
      return sizeof(uint32_t);
    case DataTypeEnum::eInt2:
      return sizeof(uint32_t) * 2;
    case DataTypeEnum::eFloat:
      return sizeof(uint32_t);
    case DataTypeEnum::eFloat2:
      return sizeof(uint32_t) * 2;
    case DataTypeEnum::eFloat3:
      return sizeof(uint32_t) * 3;
    case DataTypeEnum::eFloat4:
      return sizeof(uint32_t) * 4;
    case DataTypeEnum::eMat4:
      return sizeof(uint32_t) * 16;
    }
    return 0;
  }

  static bool isScalar(type const& a) noexcept
  {
    return !(std::holds_alternative<Source>(a) || std::holds_alternative<BufferRef>(a));
  }
};

using Parameter = ParamHelper::type;

} // namespace terra