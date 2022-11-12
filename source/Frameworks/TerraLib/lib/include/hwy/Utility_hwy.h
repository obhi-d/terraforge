#include <hwy/NodeMeta_hwy.h>
#include <hwy/contrib/math/math-inl.h>
#include <hwy/highway.h>

namespace terra::HWY_NAMESPACE
{
namespace hn = hwy::HWY_NAMESPACE;

// I i : int type
// V v : float vector
// M m : Mask
// using mask32v  = hn::Mask<hn::ScalableTag<uint32_t>>;
// using int32v   = hn::Vec<hn::ScalableTag<int32_t>>;
// using float32v = hn::Vec<hn::ScalableTag<float>>;

using M_t = hn::ScalableTag<uint32_t>;
using V_t = hn::ScalableTag<float>;
using I_t = hn::ScalableTag<int>;

constexpr float F2 = 0.366025403f;
constexpr float G2 = 0.211324865f;
/* Skewing factors for 3D simplex grid:
 * F3 = 1/3
 * G3 = 1/6 */
constexpr float F3 = 0.333333333f;
constexpr float G3 = 0.166666667f;

// The skewing and unskewing factors are hairy again for the 4D case
constexpr float F4 = 0.309016994f; // F4 = (Math.sqrt(5.0)-1.0)/4.0
constexpr float G4 = 0.138196601f; // G4 = (5.0-Math.sqrt(5.0))/20.0

HWY_API auto int32v(int32_t i)
{
  I_t const itag{};
  return hn::Set(itag, i);
}

HWY_API auto float32v(float i)
{
  V_t const vtag{};
  return hn::Set(vtag, i);
}

template <typename I>
HWY_API auto hashPrimes(I seed, I x, I y)
{
  const hn::ScalableTag<int32_t> d;

  auto hash = hn::Xor(seed, x);
  hash      = hn::Xor(hash, y);
  hash      = hn::Mul(hash, hn::Set(d, 0x27d4eb2d));
  return hn::Xor(hn::ShiftRight<15>(hash), hash);
}

template <typename I, typename V>
HWY_API auto gradientDotFancy(I hash, V x, V y)
{
  const hn::ScalableTag<int32_t> d;
  const hn::ScalableTag<float>   v;

  auto index =
    hn::ConvertTo(d, hn::Mul(hn::ConvertTo(v, hn::And(hash, hn::Set(d, 0x3FFFFF))), hn::Set(v, 1.3333333333333333f)));
  auto xy = hn::RebindMask(v, hn::ShiftLeft<29>(index) != hn::Set(d, 0));

  auto a = hn::IfThenElse(xy, y, x);
  auto b = hn::IfThenElse(xy, x, y);

  b        = hn::BitCast(v, hn::Xor(hn::BitCast(d, b), hn::ShiftLeft<31>(index)));
  auto ax2 = hn::RebindMask(v, hn::ShiftRight<31>(hn::ShiftLeft<30>(index)) != hn::Set(d, 0));

  a = hn::Mul(a, hn::IfThenElse(ax2, hn::Set(v, 2.0f), hn::Set(v, terra::consts::root3)));
  b = hn::IfThenZeroElse(ax2, b);

  return hn::BitCast(v, hn::Xor(hn::BitCast(d, hn::Add(a, b)), hn::ShiftRight<31>(hn::ShiftRight<3>(index))));
}

template <typename I, typename V>
HWY_API auto gradientDot(I hash, V x, V y)
{
  const hn::DFromV<I>          d;
  const hn::ScalableTag<float> v;

  auto bit1 = hn::ShiftLeft<31>(hash);
  auto bit2 = hn::ShiftLeft<31>(hn::ShiftRight<1>(hash));
  auto bit4 = hn::RebindMask(v, hn::ShiftLeft<29>(hash) != hn::Set(hn::DFromV<I>(), 0));

  x = hn::BitCast(v, hn::Xor(hn::BitCast(d, x), bit1));
  y = hn::BitCast(v, hn::Xor(hn::BitCast(d, y), bit2));

  auto a = hn::IfThenElse(bit4, y, x);
  auto b = hn::IfThenElse(bit4, x, y);

  return hn::MulAdd(hn::Set(v, 1.0f + terra::consts::root2), a, b);
}

// Vector builders

/// <summary>
/// Vector with values incrementing from 0 based on element index {0, 1, 2, 3...}
/// </summary>
/// <code>
/// example: int32v::FS_Incremented()
/// </code>
template <typename V>
HWY_API auto FS_Incremented(V v)
{
  return hn::Iota(v, 0);
}

// Extract
/// <summary>
/// Retreive element 0 from vector
/// </summary>
/// <code>
/// float FS_Extract0_f32( float32v f )
/// </code>
template <typename V>
HWY_API auto FS_Extract0_f32(V v)
{
  return hn::GetLane(v);
}

/// <summary>
/// Retreive element 0 from vector
/// </summary>
/// <code>
/// int32_t FS_Extract0_i32( int32v i )
/// </code>
template <typename V>
HWY_API auto FS_Extract0_i32(V v)
{
  return hn::GetLane(v);
}

/// <summary>
/// Retreive element from vector at position
/// </summary>
/// <code>
/// float FS_Extract_f32( float32v f, size_t idx )
/// </code>
template <typename V>
HWY_API auto FS_Extract_f32(V v, size_t l)
{
  return hn::ExtractLane(v, l);
}

/// <summary>
/// Retreive element from vector at position
/// </summary>
/// <code>
/// int32_t FS_Extract_i32( int32v i, size_t idx )
/// </code>
template <typename V>
HWY_API auto FS_Extract_i32(V v, size_t l)
{
  return hn::ExtractLane(v, l);
}

// Cast

/// <summary>
/// Bitwise cast int to float
/// </summary>
/// <code>
/// float32v FS_Casti32_f32( int32v i )
/// </code>
template <typename V>
HWY_API auto FS_Casti32_f32(V v)
{
  const hn::ScalableTag<float> D;
  return hn::BitCast(D, v);
}

/// <summary>
/// Bitwise cast float to int
/// </summary>
/// <code>
/// int32v FS_Castf32_i32( float32v f )
/// </code>
template <typename V>
HWY_API auto FS_Castf32_i32(V v)
{
  const hn::ScalableTag<int32_t> D;
  return hn::BitCast(D, v);
}

// Convert

/// <summary>
/// Convert int to float
/// </summary>
/// <remarks>
/// Rounding: truncate
/// </remarks>
/// <code>
/// float32v FS_Converti32_f32( int32v i )
/// </code>
template <typename V>
HWY_API auto FS_Converti32_f32(V v)
{
  const hn::ScalableTag<float> D;
  return hn::ConvertTo(D, v);
}

/// <summary>
/// Convert float to int
/// </summary>
/// <code>
/// int32v FS_Convertf32_i32( float32v f )
/// </code>
template <typename V>
HWY_API auto FS_Convertf32_i32(V v)
{
  const hn::ScalableTag<int32_t> D;
  return hn::ConvertTo(D, v);
}

// Select

/// <summary>
/// return ( m ? a : b )
/// </summary>
/// <code>
/// float32v FS_Select_f32( mask32v m, float32v a, float32v b )
/// </code>
template <typename M, typename V>
HWY_API auto FS_Select_f32(M m, V a, V b)
{
  return hn::IfThenElse(m, a, b);
}

/// <summary>
/// return ( m ? a : b )
/// </summary>
/// <code>
/// int32v FS_Select_i32( mask32v m, int32v a, int32v b )
/// </code>
template <typename M, typename V>
HWY_API auto FS_Select_i32(M m, V a, V b)
{
  return hn::IfThenElse(m, a, b);
}

// Min, Max

/// <summary>
/// return ( a < b ? a : b )
/// </summary>
/// <code>
/// float32v FS_Min_f32( float32v a, float32v b )
/// </code>
template <typename V>
HWY_API auto FS_Min_f32(V a, V b)
{
  return hn::Min(a, b);
}

/// <summary>
/// return ( a > b ? a : b )
/// </summary>
/// <code>
/// float32v FS_Max_f32( float32v a, float32v b )
/// </code>
template <typename V>
HWY_API auto FS_Max_f32(V a, V b)
{
  return hn::Max(a, b);
}

/// <summary>
/// return ( a < b ? a : b )
/// </summary>
/// <code>
/// int32v FS_Min_i32( int32v a, int32v b )
/// </code>
template <typename V>
HWY_API auto FS_Min_i32(V a, V b)
{
  return hn::Min(a, b);
}

/// <summary>
/// return ( a > b ? a : b )
/// </summary>
/// <code>
/// int32v FS_Max_i32( int32v a, int32v b )
/// </code>
template <typename V>
HWY_API auto FS_Max_i32(V a, V b)
{
  return hn::Max(a, b);
}

// Bitwise

/// <summary>
/// return ( a & ~b )
/// </summary>
/// <code>
/// float32v FS_BitwiseAndNot_f32( float32v a, float32v b )
/// </code>
template <typename T>
HWY_API auto FS_BitwiseAndNot_f32(T a, T b)
{
  const hn::ScalableTag<float>   V;
  const hn::ScalableTag<int32_t> D;
  return hn::BitCast(V, hn::AndNot(hn::BitCast(D, b), hn::BitCast(D, a)));
}

/// <summary>
/// return ( a & ~b )
/// </summary>
/// <code>
/// int32v FS_BitwiseAndNot_i32( int32v a, int32v b )
/// </code>
template <typename T>
HWY_API auto FS_BitwiseAndNot_i32(T a, T b)
{
  return hn::AndNot(b, a);
}

/// <summary>
/// return ( a & ~b )
/// </summary>
/// <code>
/// mask32v FS_BitwiseAndNot_m32( mask32v a, mask32v b )
/// </code>
template <typename M>
HWY_API auto FS_BitwiseAndNot_m32(M a, M b)
{
  return hn::AndNot(b, a);
}

/// <summary>
/// return ZeroExtend( a >> b )
/// </summary>
/// <code>
/// float32v FS_BitwiseShiftRightZX_f32( float32v a, int32_t b )
/// </code>
template <int32_t b, typename V>
HWY_API auto FS_BitwiseShiftRightZX_f32(V a)
{
  const hn::ScalableTag<float>   D;
  const hn::ScalableTag<int32_t> I;
  return hn::BitCast(D, hn::ShiftRight<b>(hn::BitCast(I, a)));
}

/// <summary>
/// return ZeroExtend( a >> b )
/// </summary>
/// <code>
/// float32v FS_BitwiseShiftRightZX_i32( int32v a, int32_t b )
/// </code>
template <int32_t b, typename I>
HWY_API auto FS_BitwiseShiftRightZX_i32(I a)
{
  return hn::ShiftRight<b>(a);
}

// Abs

/// <summary>
/// return ( a < 0 ? -a : a )
/// </summary>
/// <code>
/// float32v FS_Abs_f32( float32v a )
/// </code>
template <typename V>
HWY_API auto FS_Abs_f32(V a)
{
  return hn::Abs(a);
}

/// <summary>
/// return ( a < 0 ? -a : a )
/// </summary>
/// <code>
/// int32v FS_Abs_i32( int32v a )
/// </code>
template <typename I>
HWY_API auto FS_Abs_i32(I a)
{
  return hn::Abs(a);
}

// Float math

/// <summary>
/// return sqrt( a )
/// </summary>
/// <code>
/// float32v FS_Sqrt_f32( float32v a )
/// </code>
template <typename V>
HWY_API auto FS_Sqrt_f32(V a)
{
  return hn::Sqrt(a);
}

/// <summary>
/// return APPROXIMATE( 1.0 / sqrt( a ) )
/// </summary>
/// <code>
/// float32v FS_InvSqrt_f32( float32v a )
/// </code>
template <typename V>
HWY_API auto FS_InvSqrt_f32(V a)
{
  return hn::ApproximateReciprocalSqrt(a);
}

/// <summary>
/// return APPROXIMATE( 1.0 / a )
/// </summary>
/// <code>
/// float32v FS_Reciprocal_f32( float32v a )
/// </code>
template <typename V>
HWY_API auto FS_Reciprocal_f32(V a)
{
  return hn::ApproximateReciprocal(a);
}

// Floor, Ceil, Round

/// <summary>
/// return floor( a )
/// </summary>
/// <remarks>
/// Rounding: Towards negative infinity
/// </remarks>
/// <code>
/// float32v FS_Floor_f32( float32v a )
/// </code>
template <typename V>
HWY_API auto FS_Floor_f32(V a)
{
  return hn::Floor(a);
}

/// <summary>
/// return ceil( a )
/// </summary>
/// <remarks>
/// Rounding: Towards positive infinity
/// </remarks>
/// <code>
/// float32v FS_Ceil_f32( float32v a )
/// </code>
template <typename V>
HWY_API auto FS_Ceil_f32(V a)
{
  return hn::Ceil(a);
}

/// <summary>
/// return round( a )
/// </summary>
/// <remarks>
/// Rounding: Banker's rounding
/// </remarks>
/// <code>
/// float32v FS_Round_f32( float32v a )
/// </code>
template <typename V>
HWY_API auto FS_Round_f32(V a)
{
  return hn::Round(a);
}

// Trig

/// <summary>
/// return APPROXIMATE( cos( a ) )
/// </summary>
/// <code>
/// float32v FS_Cos_f32( float32v a )
/// </code>
template <typename V>
HWY_API auto FS_Cos_f32(V a)
{
  return hn::Cos(a);
}

/// <summary>
/// return APPROXIMATE( sin( a ) )
/// </summary>
/// <code>
/// float32v FS_Sin_f32( float32v a )
/// </code>
template <typename V>
HWY_API auto FS_Sin_f32(V a)
{
  return hn::Sin(a);
}

// Math

/// <summary>
/// return log( a )
/// </summary>
/// <remarks>
/// a <= 0 returns 0
/// </remarks>
/// <code>
/// float32v FS_Log_f32( float32v a )
/// </code>
template <typename V>
HWY_API auto FS_Log_f32(V a)
{
  return hn::Log(a);
}

/// <summary>
/// return exp( a )
/// </summary>
/// <remarks>
/// a will be clamped to -88.376, 88.376
/// </remarks>
/// <code>
/// float32v FS_Exp_f32( float32v a )
/// </code>
template <typename V>
HWY_API auto FS_Exp_f32(V a)
{
  return hn::Exp(hn::DFromV<V>(), a);
}

// Mask

/// <summary>
/// return ( m ? a : 0 )
/// </summary>
/// <code>
/// int32v FS_Mask_i32( int32v a, mask32v m )
/// </code>
template <typename I, typename M>
HWY_API auto FS_Mask_i32(I a, M m)
{
  return hn::IfThenElseZero(m, a);
}

/// <summary>
/// return ( m ? a : 0 )
/// </summary>
/// <code>
/// float32v FS_Mask_f32( float32v a, mask32v m )
/// </code>
template <typename V, typename M>
HWY_API auto FS_Mask_f32(V a, M m)
{
  return hn::IfThenElseZero(m, a);
}

/// <summary>
/// return ( m ? 0 : a )
/// </summary>
/// <code>
/// int32v FS_NMask_i32( int32v a, mask32v m )
/// </code>
template <typename I, typename M>
HWY_API auto FS_NMask_i32(I a, M m)
{
  return hn::IfThenZeroElse(m, a);
}

/// <summary>
/// return ( m ? 0 : a )
/// </summary>
/// <code>
/// float32v FS_NMask_f32( float32v a, mask32v m )
/// </code>
template <typename V, typename M>
HWY_API auto FS_NMask_f32(V a, M m)
{
  return hn::IfThenZeroElse(m, a);
}

/// <summary>
/// return m.contains( true )
/// </summary>
/// <code>
/// bool FS_AnyMask_bool( mask32v m )
/// </code>
template <typename M>
HWY_API auto FS_AnyMask_bool(M m)
{
  return !hn::AllFalse(m);
}

// FMA

/// <summary>
/// return ( (a * b) + c )
/// </summary>
/// <code>
/// float32v FS_FMulAdd_f32( float32v a, float32v b, float32v c )
/// </code>
template <typename V>
HWY_API auto FS_FMulAdd_f32(V a, V b, V c)
{
  return hn::MulAdd(a, b, c);
}

/// <summary>
/// return ( -(a * b) + c )
/// </summary>
/// <code>
/// float32v FS_FNMulAdd_f32( float32v a, float32v b, float32v c )
/// </code>
template <typename V>
HWY_API auto FS_FNMulAdd_f32(V a, V b, V c)
{
  return hn::NegMulAdd(a, b, c);
}

// Masked float

/// <summary>
/// return ( m ? (a + b) : a )
/// </summary>
/// <code>
/// float32v FS_MaskedAdd_f32( float32v a, float32v b, mask32v m )
/// </code>
template <typename V, typename M>
HWY_API auto FS_MaskedAdd_f32(V a, V b, M m)
{
  return hn::Add(hn::IfThenElseZero(hn::RebindMask(hn::DFromV<V>(), m), b), a);
}

/// <summary>
/// return ( m ? (a - b) : a )
/// </summary>
/// <code>
/// float32v FS_MaskedSub_f32( float32v a, float32v b, mask32v m )
/// </code>
template <typename V, typename M>
HWY_API auto FS_MaskedSub_f32(V a, V b, M m)
{
  return hn::Sub(a, hn::IfThenElseZero(hn::RebindMask(hn::DFromV<V>(), m), b));
}

/// <summary>
/// return ( m ? (a * b) : a )
/// </summary>
/// <code>
/// float32v FS_MaskedMul_f32( float32v a, float32v b, mask32v m )
/// </code>
template <typename V, typename M>
HWY_API auto FS_MaskedMul_f32(V a, V b, M m)
{
  return hn::Mul(hn::IfThenElseZero(hn::RebindMask(hn::DFromV<V>(), m), b), a);
}

// Masked int32

/// <summary>
/// return ( m ? (a + b) : a )
/// </summary>
/// <code>
/// int32v FS_MaskedAdd_i32( int32v a, int32v b, mask32v m )
/// </code>
template <typename I, typename M>
HWY_API auto FS_MaskedAdd_i32(I a, I b, M m)
{
  return hn::Add(hn::IfThenElseZero(hn::RebindMask(hn::DFromV<I>(), m), b), a);
}

/// <summary>
/// return ( m ? (a - b) : a )
/// </summary>
/// <code>
/// int32v FS_MaskedSub_i32( int32v a, int32v b, mask32v m )
/// </code>
template <typename I, typename M>
HWY_API auto FS_MaskedSub_i32(I a, I b, M m)
{
  return hn::Sub(a, hn::IfThenElseZero(hn::RebindMask(hn::DFromV<I>(), m), b));
}

/// <summary>
/// return ( m ? (a * b) : a )
/// </summary>
/// <code>
/// int32v FS_MaskedMul_i32( int32v a, int32v b, mask32v m )
/// </code>
template <typename I, typename M>
HWY_API auto FS_MaskedMul_i32(I a, I b, M m)
{
  return hn::Mul(hn::IfThenElseZero(hn::RebindMask(hn::DFromV<I>(), m), b), a);
}

/// <summary>
/// return ( m ? (a + 1) : a )
/// </summary>
/// <code>
/// int32v FS_MaskedIncrement_i32( int32v a, mask32v m )
/// </code>
template <typename I, typename M>
HWY_API auto FS_MaskedIncrement_i32(I a, M m)
{
  return hn::Add(hn::IfThenElseZero(hn::RebindMask(hn::DFromV<I>(), m), hn::Set(hn::DFromV<I>{}, 1)), a);
}
/// <summary>
/// return ( m ? (a - 1) : a )
/// </summary>
/// <code>
/// int32v FS_MaskedDecrement_i32( int32v a, mask32v m )
/// </code>
template <typename I, typename M>
HWY_API auto FS_MaskedDecrement_i32(I a, M m)
{
  return hn::Sub(a, hn::IfThenElseZero(hn::RebindMask(hn::DFromV<I>(), m), hn::Set(hn::DFromV<I>{}, 1)));
}
// NMasked float

/// <summary>
/// return ( m ? a : (a + b) )
/// </summary>
/// <code>
/// float32v FS_NMaskedAdd_f32( float32v a, float32v b, mask32v m )
/// </code>
template <typename V, typename M>
HWY_API auto FS_NMaskedAdd_f32(V a, V b, M m)
{
  return hn::Add(hn::IfThenZeroElse(hn::RebindMask(hn::DFromV<V>(), m), b), a);
}

/// <summary>
/// return ( m ? a : (a - b) )
/// </summary>
/// <code>
/// float32v FS_NMaskedSub_f32( float32v a, float32v b, mask32v m )
/// </code>
//#define FS_NMaskedSub_f32(...) FastSIMD::NMaskedSub_f32<FS>(__VA_ARGS__)
template <typename V, typename M>
HWY_API auto FS_NMaskedSub_f32(V a, V b, M m)
{
  return hn::Sub(a, hn::IfThenZeroElse(hn::RebindMask(hn::DFromV<V>(), m), b));
}

/// <summary>
/// return ( m ? a : (a * b) )
/// </summary>
/// <code>
/// float32v FS_NMaskedMul_f32( float32v a, float32v b, mask32v m )
/// </code>
template <typename V, typename M>
HWY_API auto FS_NMaskedMul_f32(V a, V b, M m)
{
  return hn::Mul(a, hn::IfThenZeroElse(hn::RebindMask(hn::DFromV<V>(), m), b));
}

// NMasked int32

/// <summary>
/// return ( m ? a : (a + b) )
/// </summary>
/// <code>
/// int32v FS_NMaskedAdd_i32( int32v a, int32v b, mask32v m )
/// </code>
template <typename I, typename M>
HWY_API auto FS_NMaskedAdd_i32(I a, I b, M m)
{
  return hn::Add(hn::IfThenZeroElse(hn::RebindMask(hn::DFromV<I>(), m), b), a);
}

/// <summary>
/// return ( m ? a : (a - b) )
/// </summary>
/// <code>
/// int32v FS_NMaskedSub_i32( int32v a, int32v b, mask32v m )
/// </code>
template <typename I, typename M>
HWY_API auto FS_NMaskedSub_i32(I a, I b, M m)
{
  return hn::Sub(a, hn::IfThenZeroElse(hn::RebindMask(hn::DFromV<I>(), m), b));
}

/// <summary>
/// return ( m ? a : (a * b) )
/// </summary>
/// <code>
/// int32v FS_NMaskedMul_i32( int32v a, int32v b, mask32v m )
/// </code>
template <typename I, typename M>
HWY_API auto FS_NMaskedMul_i32(I a, I b, M m)
{
  return hn::Mul(a, hn::IfThenZeroElse(hn::RebindMask(hn::DFromV<I>(), m), b));
}

/// <summary>
/// return pow( v, pow )
/// </summary>
/// <code>
/// float32v FS_Pow_f32( float32v v, float32v pow )
/// </code>
// #define FS_Pow_f32(...) FastSIMD::Pow_f32<FS>(__VA_ARGS__)
template <typename V>
HWY_API auto FS_Pow_f32(V value, V pow)
{
  return FS_Exp_f32(hn::Mul(pow, FS_Log_f32(value)));
}

template <typename V>
HWY_API auto FS_SubMul(V value, V offset, V rsize)
{
  return hn::Mul(hn::Sub(value, offset), rsize);
}

enum DistanceFunc
{
  eEucledian,
  eEucledianSq,
  eManhattan,
  eHybrid,
  eMaxAxis
};

template <int DistTy, typename V>
HWY_API auto Distance(V dx, V dy)
{
  constexpr DistanceFunc DistanceTy = (DistanceFunc)DistTy;
  if constexpr (DistanceTy == DistanceFunc::eEucledianSq)
    return hn::MulAdd(dx, dx, hn::Mul(dy, dy));
  else if constexpr (DistanceTy == DistanceFunc::eManhattan)
    return hn::Add(hn::Abs(dx), hn::Abs(dy));
  else if constexpr (DistanceTy == DistanceFunc::eHybrid)
    return hn::Add(hn::MulAdd(dx, dx, hn::Abs(dx)), hn::MulAdd(dy, dy, hn::Abs(dy)));
  else if constexpr (DistanceTy == DistanceFunc::eMaxAxis)
    return hn::Max(dx, dy);
  else
    return hn::Sqrt(hn::MulAdd(dx, dx, hn::Mul(dy, dy)));
}

template <typename I, typename V>
inline V grad(I hash, V x, V y)
{
  hn::DFromV<I> itag{};
  hn::DFromV<V> vtag{};
  const auto    one = hn::Set(itag, 1);
  auto const    two = hn::Set(itag, 2);
  auto const    h   = hn::And(hash, hn::Set(itag, 7));                             // Convert low 3 bits of hash code
  auto const u = hn::IfThenElse(hn::RebindMask(vtag, h < hn::Set(itag, 4)), x, y); // into 8 simple gradient directions,
  auto const v = hn::IfThenElse(hn::RebindMask(vtag, h < hn::Set(itag, 4)), y, x);
  return hn::IfThenElse(hn::RebindMask(vtag, hn::And(h, one) == one), hn::Neg(u), u) +
         hn::IfThenElse(hn::RebindMask(vtag, hn::And(h, two) == two), hn::Set(vtag, -2.f) * v, hn::Set(vtag, 2.f) * v);
}
template <typename V>
inline V interpHermite(V t)
{
  return t * t * FS_FNMulAdd_f32(t, float32v(2), float32v(3));
}

static inline constexpr std::array<float, 8> grad2lutX = {-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f};
static inline constexpr std::array<float, 8> grad2lutY = {-1.0f, 0.0f, 0.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f};

template <typename V>
struct XY
{
  V x;
  V y;
};

template <typename I, typename V>
inline XY<V> gradrotXY(I idx, V sin, V cos)
{
  hn::DFromV<V> vtag{};
  XY<V>         xy;
  auto          gx0 = hn::GatherIndex(vtag, grad2lutX.data(), idx);
  auto          gy0 = hn::GatherIndex(vtag, grad2lutX.data(), idx);
  xy.x              = hn::MulSub(cos, gx0, sin * gy0);
  xy.y              = hn::MulAdd(sin, gx0, cos * gy0);
  return xy;
}

template <typename V>
inline V graddotp2(V gx, V gy, V x, V y)
{
  return hn::MulAdd(gx, x, gy * y);
}

template <typename V>
inline V graddotp2(XY<V> g, V x, V y)
{
  return hn::MulAdd(g.x, x, g.y * y);
}

template <typename V>
inline V lerp(V a, V b, V t)
{
  return FS_FMulAdd_f32(t, b - a, a);
}

template <typename V>
struct DerivNoise
{
  V h;
  V dx;
  V dy;

  DerivNoise(V ih, V idx, V idy) : h(ih), dx(idx), dy(idy) {}
};

template <typename V, typename C>
inline DerivNoise<V> dnoise(V x, V y, C const& perm)
{
  I_t           itag{};
  hn::DFromV<V> vtag{};

  // float n0, n1, n2; // Noise contributions from the three corners

  // Skew the input space to determine which simplex cell we're in
  auto s  = (x + y) * float32v(F2); // Hairy factor for 2D
  auto xs = x + s;
  auto ys = y + s;
  auto i  = hn::Floor(xs);
  auto j  = hn::Floor(ys);

  auto t  = (i + j) * float32v(G2);
  auto X0 = i - t; // Unskew the cell origin back to (x,y) space
  auto Y0 = j - t;
  auto x0 = x - X0; // The x,y distances from the cell origin
  auto y0 = y - Y0;

  auto i1 = hn::IfThenElse(x0 > y0, hn::Set(vtag, 1.0f), hn::Zero(vtag));
  auto j1 = hn::IfThenElse(x0 > y0, hn::Zero(vtag), hn::Set(vtag, 1.0f));

  // A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and
  // a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where
  // c = (3-sqrt(3))/6

  auto x1 = x0 - i1 + hn::Set(vtag, G2); // Offsets for middle corner in (x,y) unskewed coords
  auto y1 = y0 - j1 + hn::Set(vtag, G2);
  auto x2 = x0 - hn::Set(vtag, 1.0f) + hn::Set(vtag, 2.0f * G2); // Offsets for last corner in (x,y) unskewed coords
  auto y2 = y0 - hn::Set(vtag, 1.0f) + hn::Set(vtag, 2.0f * G2);
  auto ti = hn::ConvertTo(itag, i);
  auto tj = hn::ConvertTo(itag, j);

  // float gx0, gy0, gx1, gy1, gx2, gy2; /* Gradients at simplex corners */
  // float t20, t40;

  /* Calculate the contribution from the three corners */
  auto t0  = float32v(0.5f) - x0 * x0 - y0 * y0;
  auto idx = hn::And(hn::GatherIndex(itag, perm.data(),
                                     hn::And(ti + hn::GatherIndex(itag, perm.data(), hn::And(tj, hn::Set(itag, 0xff))),
                                             hn::Set(itag, 0xff))),
                     hn::Set(itag, 7));

  auto gx0 = hn::IfNegativeThenElse(t0, float32v(0.f), hn::GatherIndex(vtag, grad2lutX.data(), idx));
  auto gy0 = hn::IfNegativeThenElse(t0, float32v(0.f), hn::GatherIndex(vtag, grad2lutY.data(), idx));
  auto t20 = hn::IfNegativeThenElse(t0, float32v(0.f), t0 * t0);
  auto t40 = t20 * t20;
  auto n0  = t40 * (gx0 * x0 + gy0 * y0);

  auto t1  = float32v(0.5f) - x1 * x1 - y1 * y1;
  idx      = hn::And(hn::GatherIndex(itag, perm.data(),
                                     hn::And(ti + hn::ConvertTo(itag, i1) +
                                               hn::GatherIndex(itag, perm.data(),
                                                               hn::And(tj + hn::ConvertTo(itag, j1), hn::Set(itag, 0xff))),
                                             hn::Set(itag, 0xff))),
                     hn::Set(itag, 7));
  auto gx1 = hn::IfNegativeThenElse(t1, float32v(0.f), hn::GatherIndex(vtag, grad2lutX.data(), idx));
  auto gy1 = hn::IfNegativeThenElse(t1, float32v(0.f), hn::GatherIndex(vtag, grad2lutY.data(), idx));
  auto t21 = hn::IfNegativeThenElse(t1, float32v(0.f), t1 * t1);
  auto t41 = t21 * t21;
  auto n1  = t41 * (gx1 * x1 + gy1 * y1);

  auto t2 = float32v(0.5f) - x2 * x2 - y2 * y2;
  idx     = hn::And(
        hn::GatherIndex(itag, perm.data(),
                        hn::And(ti + hn::Set(itag, 1) +
                                  hn::GatherIndex(itag, perm.data(), hn::And(tj + hn::Set(itag, 1), hn::Set(itag, 0xff))),
                                hn::Set(itag, 0xff))),
        hn::Set(itag, 7));
  auto gx2 = hn::IfNegativeThenElse(t2, float32v(0.f), hn::GatherIndex(vtag, grad2lutX.data(), idx));
  auto gy2 = hn::IfNegativeThenElse(t2, float32v(0.f), hn::GatherIndex(vtag, grad2lutY.data(), idx));
  auto t22 = hn::IfNegativeThenElse(t2, float32v(0.f), t2 * t2);
  auto t42 = t22 * t22;
  auto n2  = t42 * (gx2 * x2 + gy2 * y2);

  /* Compute derivative, if requested by supplying non-null pointers
   * for the last two arguments */
  /*  A straight, unoptimised calculation would be like:
   *    *dnoise_dx = -8.0f * t20 * t0 * x0 * ( gx0 * x0 + gy0 * y0 ) + t40 * gx0;
   *    *dnoise_dy = -8.0f * t20 * t0 * y0 * ( gx0 * x0 + gy0 * y0 ) + t40 * gy0;
   *    *dnoise_dx += -8.0f * t21 * t1 * x1 * ( gx1 * x1 + gy1 * y1 ) + t41 * gx1;
   *    *dnoise_dy += -8.0f * t21 * t1 * y1 * ( gx1 * x1 + gy1 * y1 ) + t41 * gy1;
   *    *dnoise_dx += -8.0f * t22 * t2 * x2 * ( gx2 * x2 + gy2 * y2 ) + t42 * gx2;
   *    *dnoise_dy += -8.0f * t22 * t2 * y2 * ( gx2 * x2 + gy2 * y2 ) + t42 * gy2;
   */
  auto temp0     = t20 * t0 * (gx0 * x0 + gy0 * y0);
  auto dnoise_dx = temp0 * x0;
  auto dnoise_dy = temp0 * y0;
  auto temp1     = t21 * t1 * (gx1 * x1 + gy1 * y1);
  dnoise_dx += temp1 * x1;
  dnoise_dy += temp1 * y1;
  auto temp2 = t22 * t2 * (gx2 * x2 + gy2 * y2);
  dnoise_dx += temp2 * x2;
  dnoise_dy += temp2 * y2;
  dnoise_dx *= float32v(-8.0f);
  dnoise_dy *= float32v(-8.0f);
  dnoise_dx += t40 * gx0 + t41 * gx1 + t42 * gx2;
  dnoise_dy += t40 * gy0 + t41 * gy1 + t42 * gy2;

  // Add contributions from each corner to get the final noise value.
  // The result is scaled to return values in the interval [-1,1].
#ifdef SIMPLEX_DERIVATIVES_RESCALE
  dnoise_dx *= 70.175438596f; /* Scale derivative to match the noise scaling */
  dnoise_dy *= 70.175438596f;
  return DerivNoise(70.175438596f * (n0 + n1 + n2), dnoise_dx, dnoise_dy); // TODO: The scale factor is preliminary!
#else
  dnoise_dx *= float32v(40.0f); /* Scale derivative to match the noise scaling */
  dnoise_dy *= float32v(40.0f);
  return DerivNoise<V>(float32v(40.0f) * (n0 + n1 + n2), dnoise_dx,
                       dnoise_dy); // TODO: The scale factor is preliminary!
#endif
}

template <typename V, typename C, typename G>
inline DerivNoise<V> dfBm(V x, V y, C const& perm, int32_t octaves, float lacunarity, float gain)
{

  auto sum  = float32v(0.0f);
  auto dx   = float32v(0.0f);
  auto dy   = float32v(0.0f);
  auto freq = float32v(1.0f);
  auto amp  = float32v(0.5f);

  for (int32_t i = 0; i < octaves; i++)
  {
    auto n = dnoise(x * freq, y * freq, perm);
    sum += n.h * amp;
    dx += n.dx * amp;
    dy += n.dy * amp;
    freq *= float32v(lacunarity);
    amp *= float32v(gain);
  }

  return DerivNoise<V>(sum, dx, dy);
}

inline void modifyDomain(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  const V_t vtag{};
  auto&     param = node.param(0);
  if (std::holds_alternative<Source>(param))
  {
    auto src = std::get<Source>(param).source;
    if (DataSource::isValid(src))
    {
      const auto lanes = (uint32)hn::Lanes(vtag);
      auto&      inp   = pipe.getInput(threadGroupId, lanes, true);
      auto&      nin   = pipe.pushInput(threadGroupId, lanes, false);
      nin              = inp;
      NodeMeta_hwy::run(src, pipe, threadGroupId, lanes);
    }
  }
}

inline void finish(Node& node, Pipeline_hwy& pipe, uint32_t threadGroupId)
{
  auto& param = node.param(0);
  if (std::holds_alternative<Source>(param))
  {
    auto src = std::get<Source>(param).source;
    if (DataSource::isValid(src))
      pipe.popInput(threadGroupId);
  }
}

template <typename I, typename V>
inline static I hashPrimesHB(I hash, V x, V y)
{
  const hn::ScalableTag<int32_t> d;
  return hn::Xor(hash, hn::Xor(hn::BitCast(d, x), hn::BitCast(d, y))) * int32v(0x27d4eb2d);
}

template <int DistTy, typename V, typename I>
inline V cellularValueNoise(I seed, V jitter, V x, V y, int valueIndex)
{
  constexpr float kJitter2D         = 0.437016f;
  constexpr int   kMaxDistanceCount = 4;

  jitter = float32v(kJitter2D) * jitter;
  std::array<V, kMaxDistanceCount> value;
  std::array<V, kMaxDistanceCount> distance;

  value.fill(float32v(INFINITY));
  distance.fill(float32v(INFINITY));

  auto xc     = FS_Convertf32_i32(x) + int32v(-1);
  auto ycBase = FS_Convertf32_i32(y) + int32v(-1);

  auto xcf     = FS_Converti32_f32(xc) - x;
  auto ycfBase = FS_Converti32_f32(ycBase) - y;

  xc *= int32v(consts::X);
  ycBase *= int32v(consts::Y);

  for (int xi = 0; xi < 3; xi++)
  {
    auto ycf = ycfBase;
    auto yc  = ycBase;
    for (int yi = 0; yi < 3; yi++)
    {
      auto hash = hashPrimesHB(seed, xc, yc);
      auto xd   = FS_Converti32_f32(hn::And(hash, int32v(0xffff))) - float32v(0xffff / 2.0f);
      auto yd   = FS_Converti32_f32(hn::And(hn::ShiftRight<16>(hash), int32v(0xffff))) - float32v(0xffff / 2.0f);

      auto invMag = jitter * FS_InvSqrt_f32(FS_FMulAdd_f32(xd, xd, yd * yd));
      xd          = FS_FMulAdd_f32(xd, invMag, xcf);
      yd          = FS_FMulAdd_f32(yd, invMag, ycf);

      auto newCellValue = float32v((float)(1.0 / INT_MAX)) * FS_Converti32_f32(hash);
      auto newDistance  = Distance<DistTy>(xd, yd);

      for (int i = 0;; i++)
      {
        auto closer         = newDistance < distance[i];
        auto localDistance  = distance[i];
        auto localCellValue = value[i];
        distance[i]         = FS_Select_f32(closer, newDistance, distance[i]);
        value[i]            = FS_Select_f32(closer, newCellValue, value[i]);

        if (i > valueIndex)
        {
          break;
        }

        newDistance  = FS_Select_f32(closer, localDistance, newDistance);
        newCellValue = FS_Select_f32(closer, localCellValue, newCellValue);
      }

      ycf += float32v(1);
      yc += int32v(consts::Y);
    }
    xcf += float32v(1);
    xc += int32v(consts::X);
  }

  return value[valueIndex];
}

template <bool WithStrength, typename V, typename I>
inline auto domainWarpInput(I seed, V warpAmp, V x, V y, V& xOut, V& yOut)
{
  auto xs = FS_Floor_f32(x);
  auto ys = FS_Floor_f32(y);

  auto x0 = FS_Convertf32_i32(xs) * int32v(terra::consts::X);
  auto y0 = FS_Convertf32_i32(ys) * int32v(terra::consts::Y);
  auto x1 = x0 + int32v(terra::consts::X);
  auto y1 = y0 + int32v(terra::consts::Y);

  xs = interpHermite(x - xs);
  ys = interpHermite(y - ys);

#define GRADIENT_COORD(_x, _y)                                                                                         \
  auto hash##_x##_y = hashPrimesHB(seed, x##_x, y##_y);                                                                \
  auto x##_x##_y    = FS_Converti32_f32(hn::And(hash##_x##_y, int32v(0xffff)));                                        \
  auto y##_x##_y    = FS_Converti32_f32(hn::And(hn::ShiftRight<16>(hash##_x##_y), int32v(0xffff)));

  GRADIENT_COORD(0, 0);
  GRADIENT_COORD(1, 0);
  GRADIENT_COORD(0, 1);
  GRADIENT_COORD(1, 1);

#undef GRADIENT_COORD

  auto normalise = float32v(1.0f / (0xffff / 2.0f));

  auto xWarp = (lerp(lerp(x00, x10, xs), lerp(x01, x11, xs), ys) - float32v(0xffff / 2.0f)) * normalise;
  auto yWarp = (lerp(lerp(y00, y10, xs), lerp(y01, y11, xs), ys) - float32v(0xffff / 2.0f)) * normalise;

  xOut = FS_FMulAdd_f32(xWarp, warpAmp, xOut);
  yOut = FS_FMulAdd_f32(yWarp, warpAmp, yOut);

  if constexpr (WithStrength)
  {
    return hn::Sqrt(FS_FMulAdd_f32(xWarp, xWarp, yWarp * yWarp));
  }
}

} // namespace terra::HWY_NAMESPACE
