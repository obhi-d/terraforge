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
  hash = hn::Xor(hash, y);
  hash = hn::Mul(hash, hn::Set(d, 0x27d4eb2d));
  return hn::Xor(hn::ShiftRight<15>(hash), hash);
}

template <typename I, typename V>
HWY_API auto gradientDotFancy(I hash, V x, V y)
{
  const hn::ScalableTag<int32_t> d;
  const hn::ScalableTag<float>    v;

  auto index =
    hn::ConvertTo(d, hn::Mul(hn::ConvertTo(v, hn::And(hash, hn::Set(d, 0x3FFFFF))), hn::Set(v, 1.3333333333333333f)));
  auto xy = hn::RebindMask(v, hn::ShiftLeft<29>(index) != hn::Set(d, 0));

  auto a = hn::IfThenElse(xy, y, x);
  auto b = hn::IfThenElse(xy, x, y);

  b        = hn::BitCast(v, hn::Xor(hn::BitCast(d, b), hn::ShiftLeft<31>(index)));
  auto ax2 = hn::RebindMask(v, hn::ShiftRight<31>(hn::ShiftLeft<30>(index)) != hn::Set(d, 0));

  a = hn::Mul(a, hn::IfThenElse(ax2, hn::Set(v, 2.0f), hn::Set(v, consts::root3)));
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

  return hn::MulAdd(hn::Set(v, 1.0f + consts::root2), a, b);
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
  const hn::ScalableTag<float> V;
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
  const hn::ScalableTag<float> D;
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
  return hn::Add(hn::IfThenElseZero(hn::RebindMask(hn::DFromV<I>(), m), hn::Set(hn::DFromV<I>, 1)), a);
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
  return hn::Sub(a, hn::IfThenElseZero(hn::RebindMask(hn::DFromV<I>(), m), hn::Set(hn::DFromV<I>, 1)));
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

} // namespace terra::HWY_NAMESPACE