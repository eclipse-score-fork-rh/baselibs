///
/// \file
/// \copyright Copyright (C) 2020-2022, Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
///

// IWYU pragma: private

#ifndef SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_AARCH64_NEON_BACKEND_HPP
#define SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_AARCH64_NEON_BACKEND_HPP

#include <score/private/bit/bit_cast.hpp>
#include <score/private/simd/abi.hpp>
#include <score/private/simd/array.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>

// usable on QNX for ASIL B software. covered by requirement broken_link_c/issue/4987269
#include <arm_neon.h> // only include NEON

// usable on QNX for ASIL B software. covered by requirement broken_link_c/issue/4049789
#define SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE __attribute__((__always_inline__))

// Documentation of Arm Neon Intrinsics
// https://arm-software.github.io/acle/neon_intrinsics/advsimd.html

namespace score::cpp
{
namespace simd
{

namespace detail
{

template <typename T>
struct neon_mask_backend;

template <>
struct neon_mask_backend<std::int32_t>
{
    using type = uint32x4_t;
    static constexpr std::size_t width{4};

    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE broadcast(const bool v) noexcept
    {
        return vdupq_n_u32(-static_cast<std::uint32_t>(v));
    }

    template <typename G, std::size_t I0, std::size_t I1, std::size_t I2, std::size_t I3>
    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE
    init(G&& gen, const std::index_sequence<I0, I1, I2, I3>) noexcept
    {
        return type{
            -static_cast<std::uint32_t>(gen(std::integral_constant<std::size_t, I0>{})),
            -static_cast<std::uint32_t>(gen(std::integral_constant<std::size_t, I1>{})),
            -static_cast<std::uint32_t>(gen(std::integral_constant<std::size_t, I2>{})),
            -static_cast<std::uint32_t>(gen(std::integral_constant<std::size_t, I3>{})),
        };
    }

    static bool SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE extract(const type v, const std::size_t i) noexcept
    {
        alignas(16) std::uint32_t tmp[4];
        vst1q_u32(&tmp[0], v);
        return tmp[i] != 0;
    }

    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE logical_not(const type v) noexcept
    {
        return vceqzq_u32(v);
    }
    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE logical_and(const type a, const type b) noexcept
    {
        return vandq_u32(a, b);
    }
    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE logical_or(const type a, const type b) noexcept
    {
        return vorrq_u32(a, b);
    }

    static bool SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE all_of(const type v) noexcept
    {
        return vget_lane_u64(vreinterpret_u64_u16(vmovn_u32(v)), 0) == 0xFFFF'FFFF'FFFF'FFFFU;
    }
    static bool SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE any_of(const type v) noexcept
    {
        return vget_lane_u64(vreinterpret_u64_u16(vmovn_u32(v)), 0) != 0;
    }
    static bool SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE none_of(const type v) noexcept
    {
        return vget_lane_u64(vreinterpret_u64_u16(vmovn_u32(v)), 0) == 0;
    }
};

template <>
struct neon_mask_backend<float>
{
    using type = uint32x4_t;
    static constexpr std::size_t width{4};

    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE broadcast(const bool v) noexcept
    {
        return vdupq_n_u32(-static_cast<std::uint32_t>(v));
    }

    template <typename G, std::size_t I0, std::size_t I1, std::size_t I2, std::size_t I3>
    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE
    init(G&& gen, const std::index_sequence<I0, I1, I2, I3>) noexcept
    {
        return type{
            -static_cast<std::uint32_t>(gen(std::integral_constant<std::size_t, I0>{})),
            -static_cast<std::uint32_t>(gen(std::integral_constant<std::size_t, I1>{})),
            -static_cast<std::uint32_t>(gen(std::integral_constant<std::size_t, I2>{})),
            -static_cast<std::uint32_t>(gen(std::integral_constant<std::size_t, I3>{})),
        };
    }

    static bool SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE extract(const type v, const std::size_t i) noexcept
    {
        alignas(16) std::uint32_t tmp[4];
        vst1q_u32(&tmp[0], v);
        return tmp[i] != 0;
    }

    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE logical_not(const type v) noexcept
    {
        return vceqzq_u32(v);
    }
    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE logical_and(const type a, const type b) noexcept
    {
        return vandq_u32(a, b);
    }
    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE logical_or(const type a, const type b) noexcept
    {
        return vorrq_u32(a, b);
    }

    static bool SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE all_of(const type v) noexcept
    {
        return vget_lane_u64(vreinterpret_u64_u16(vmovn_u32(v)), 0) == 0xFFFF'FFFF'FFFF'FFFFU;
    }
    static bool SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE any_of(const type v) noexcept
    {
        return vget_lane_u64(vreinterpret_u64_u16(vmovn_u32(v)), 0) != 0;
    }
    static bool SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE none_of(const type v) noexcept
    {
        return vget_lane_u64(vreinterpret_u64_u16(vmovn_u32(v)), 0) == 0;
    }
};

template <>
struct neon_mask_backend<double>
{
    using type = uint64x2_t;
    static constexpr std::size_t width{2};

    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE broadcast(const bool v) noexcept
    {
        return vdupq_n_u64(-static_cast<std::uint64_t>(v));
    }

    template <typename G, std::size_t I0, std::size_t I1>
    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE init(G&& gen,
                                                                      const std::index_sequence<I0, I1>) noexcept
    {
        return type{
            -static_cast<std::uint64_t>(gen(std::integral_constant<std::size_t, I0>{})),
            -static_cast<std::uint64_t>(gen(std::integral_constant<std::size_t, I1>{})),
        };
    }

    static bool SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE extract(const type v, const std::size_t i) noexcept
    {
        alignas(16) std::uint64_t tmp[2];
        vst1q_u64(&tmp[0], v);
        return tmp[i] != 0;
    }

    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE logical_not(const type v) noexcept
    {
        return vceqzq_u64(v);
    }
    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE logical_and(const type a, const type b) noexcept
    {
        return vandq_u64(a, b);
    }
    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE logical_or(const type a, const type b) noexcept
    {
        return vorrq_u64(a, b);
    }

    static bool SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE all_of(const type v) noexcept
    {
        return vget_lane_u64(vreinterpret_u64_u32(vmovn_u64(v)), 0) == 0xFFFF'FFFF'FFFF'FFFFU;
    }

    static bool SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE any_of(const type v) noexcept
    {
        return vget_lane_u64(vreinterpret_u64_u32(vmovn_u64(v)), 0) != 0;
    }
    static bool SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE none_of(const type v) noexcept
    {
        return vget_lane_u64(vreinterpret_u64_u32(vmovn_u64(v)), 0) == 0;
    }
};

template <typename T>
struct neon_backend;

template <>
struct neon_backend<std::int32_t>
{
    using type = int32x4_t;
    using mask_type = neon_mask_backend<std::int32_t>::type;
    static constexpr std::size_t width{4};

    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE broadcast(const std::int32_t v) noexcept
    {
        return vdupq_n_s32(v);
    }

    template <typename G, std::size_t I0, std::size_t I1, std::size_t I2, std::size_t I3>
    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE
    init(G&& gen, const std::index_sequence<I0, I1, I2, I3>) noexcept
    {
        return type{
            gen(std::integral_constant<std::size_t, I0>{}),
            gen(std::integral_constant<std::size_t, I1>{}),
            gen(std::integral_constant<std::size_t, I2>{}),
            gen(std::integral_constant<std::size_t, I3>{}),
        };
    }

    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE load(const std::int32_t* const v) noexcept
    {
        return vld1q_s32(v);
    }
    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE load_aligned(const std::int32_t* const v) noexcept
    {
        return vld1q_s32(v);
    }
    static void SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE store(std::int32_t* const v, const type a) noexcept
    {
        vst1q_s32(v, a);
    }
    static void SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE store_aligned(std::int32_t* const v,
                                                                               const type a) noexcept
    {
        vst1q_s32(v, a);
    }

    static std::int32_t SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE extract(const type v,
                                                                                 const std::size_t i) noexcept
    {
        alignas(16) std::int32_t tmp[width];
        vst1q_s32(&tmp[0], v);
        return tmp[i];
    }

    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE add(const type a, const type b) noexcept
    {
        return vaddq_s32(a, b);
    }
    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE subtract(const type a, const type b) noexcept
    {
        return vsubq_s32(a, b);
    }
    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE multiply(const type a, const type b) noexcept
    {
        return vmulq_s32(a, b);
    }
    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE divide(const type a, const type b) noexcept
    {
        type r0{};
        r0 = vsetq_lane_s32(vgetq_lane_s32(a, 0) / vgetq_lane_s32(b, 0), r0, 0);
        r0 = vsetq_lane_s32(vgetq_lane_s32(a, 1) / vgetq_lane_s32(b, 1), r0, 1);

        type r1{};
        r1 = vsetq_lane_s32(vgetq_lane_s32(a, 2) / vgetq_lane_s32(b, 2), r1, 0);
        r1 = vsetq_lane_s32(vgetq_lane_s32(a, 3) / vgetq_lane_s32(b, 3), r1, 1);

        return vcombine_s32(vget_low_s32(r0), vget_low_s32(r1));
    }

    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE negate(const type v) noexcept { return vnegq_s32(v); }

    static mask_type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE equal(const type a, const type b) noexcept
    {
        return vceqq_s32(a, b);
    }
    static mask_type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE not_equal(const type a, const type b) noexcept
    {
        return vmvnq_u32(vceqq_s32(a, b));
    }
    static mask_type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE less_than(const type a, const type b) noexcept
    {
        return vcltq_s32(a, b);
    }
    static mask_type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE less_equal(const type a, const type b) noexcept
    {
        return vcleq_s32(a, b);
    }
    static mask_type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE greater_than(const type a, const type b) noexcept
    {
        return vcgtq_s32(a, b);
    }
    static mask_type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE greater_equal(const type a, const type b) noexcept
    {
        return vcgeq_s32(a, b);
    }

    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE min(const type a, const type b) noexcept
    {
        return vminq_s32(a, b);
    }
    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE max(const type a, const type b) noexcept
    {
        return vmaxq_s32(a, b);
    }

    static float32x4_t SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE convert(const type v, float) noexcept
    {
        return vcvtq_f32_s32(v);
    }

    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE blend(const type a,
                                                                       const type b,
                                                                       const mask_type c) noexcept
    {
        return vbslq_s32(c, b, a);
    }
};

template <>
struct neon_backend<float>
{
    using type = float32x4_t;
    using mask_type = neon_mask_backend<float>::type;
    static constexpr std::size_t width{4};

    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE broadcast(const float v) noexcept
    {
        return vdupq_n_f32(v);
    }

    template <typename G, std::size_t I0, std::size_t I1, std::size_t I2, std::size_t I3>
    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE
    init(G&& gen, const std::index_sequence<I0, I1, I2, I3>) noexcept
    {
        return type{
            gen(std::integral_constant<std::size_t, I0>{}),
            gen(std::integral_constant<std::size_t, I1>{}),
            gen(std::integral_constant<std::size_t, I2>{}),
            gen(std::integral_constant<std::size_t, I3>{}),
        };
    }

    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE load(const float* const v) noexcept
    {
        return vld1q_f32(v);
    }
    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE load_aligned(const float* const v) noexcept
    {
        return vld1q_f32(v);
    }
    static void SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE store(float* const v, const type a) noexcept
    {
        vst1q_f32(v, a);
    }
    static void SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE store_aligned(float* const v, const type a) noexcept
    {
        vst1q_f32(v, a);
    }

    static float SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE extract(const type v, const std::size_t i) noexcept
    {
        alignas(16) float tmp[width];
        vst1q_f32(&tmp[0], v);
        return tmp[i];
    }

    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE add(const type a, const type b) noexcept
    {
        return vaddq_f32(a, b);
    }
    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE subtract(const type a, const type b) noexcept
    {
        return vsubq_f32(a, b);
    }
    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE multiply(const type a, const type b) noexcept
    {
        return vmulq_f32(a, b);
    }
    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE divide(const type a, const type b) noexcept
    {
        return vdivq_f32(a, b);
    }
    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE negate(const type v) noexcept { return vnegq_f32(v); }

    static mask_type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE equal(const type a, const type b) noexcept
    {
        return vceqq_f32(a, b);
    }
    static mask_type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE not_equal(const type a, const type b) noexcept
    {
        return vmvnq_u32(vceqq_f32(a, b));
    }
    static mask_type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE less_than(const type a, const type b) noexcept
    {
        return vcltq_f32(a, b);
    }
    static mask_type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE less_equal(const type a, const type b) noexcept
    {
        return vcleq_f32(a, b);
    }
    static mask_type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE greater_than(const type a, const type b) noexcept
    {
        return vcgtq_f32(a, b);
    }
    static mask_type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE greater_equal(const type a, const type b) noexcept
    {
        return vcgeq_f32(a, b);
    }

    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE min(const type a, const type b) noexcept
    {
        return vbslq_f32(vcltq_f32(b, a), b, a);
    }
    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE max(const type a, const type b) noexcept
    {
        return vbslq_f32(vcltq_f32(a, b), b, a);
    }

    static int32x4_t SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE convert(const type v, std::int32_t) noexcept
    {
        return vcvtq_s32_f32(v);
    }

    static mask_type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE is_nan(const type v) noexcept
    {
        const auto inf = score::cpp::bit_cast<std::uint32_t>(std::numeric_limits<float>::infinity());
        const auto abs_v = vandq_u32(vreinterpretq_u32_f32(v), vdupq_n_u32(0x7FFF'FFFFU));
        return vcltq_u32(vdupq_n_u32(inf), abs_v);
    }

    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE blend(const type a,
                                                                       const type b,
                                                                       const mask_type c) noexcept
    {
        return vbslq_f32(c, b, a);
    }
};

template <>
struct neon_backend<double>
{
    using type = float64x2_t;
    using mask_type = neon_mask_backend<double>::type;
    static constexpr std::size_t width{2};

    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE broadcast(const double v) noexcept
    {
        return vdupq_n_f64(v);
    }

    template <typename G, std::size_t I0, std::size_t I1>
    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE init(G&& gen,
                                                                      const std::index_sequence<I0, I1>) noexcept
    {
        return type{
            gen(std::integral_constant<std::size_t, I0>{}),
            gen(std::integral_constant<std::size_t, I1>{}),
        };
    }

    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE load(const double* const v) noexcept
    {
        return vld1q_f64(v);
    }
    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE load_aligned(const double* const v) noexcept
    {
        return vld1q_f64(v);
    }
    static void SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE store(double* const v, const type a) noexcept
    {
        vst1q_f64(v, a);
    }
    static void SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE store_aligned(double* const v, const type a) noexcept
    {
        vst1q_f64(v, a);
    }

    static double SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE extract(const type v, const std::size_t i) noexcept
    {
        alignas(16) double tmp[width];
        vst1q_f64(&tmp[0], v);
        return tmp[i];
    }

    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE add(const type a, const type b) noexcept
    {
        return vaddq_f64(a, b);
    }
    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE subtract(const type a, const type b) noexcept
    {
        return vsubq_f64(a, b);
    }
    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE multiply(const type a, const type b) noexcept
    {
        return vmulq_f64(a, b);
    }
    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE divide(const type a, const type b) noexcept
    {
        return vdivq_f64(a, b);
    }
    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE negate(const type v) noexcept { return vnegq_f64(v); }

    static mask_type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE equal(const type a, const type b) noexcept
    {
        return vceqq_f64(a, b);
    }
    static mask_type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE not_equal(const type a, const type b) noexcept
    {
        return vbicq_u64(vdupq_n_u64(0xFFFF'FFFF'FFFF'FFFF), vceqq_f64(a, b));
    }
    static mask_type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE less_than(const type a, const type b) noexcept
    {
        return vcltq_f64(a, b);
    }
    static mask_type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE less_equal(const type a, const type b) noexcept
    {
        return vcleq_f64(a, b);
    }
    static mask_type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE greater_than(const type a, const type b) noexcept
    {
        return vcgtq_f64(a, b);
    }
    static mask_type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE greater_equal(const type a, const type b) noexcept
    {
        return vcgeq_f64(a, b);
    }

    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE min(const type a, const type b) noexcept
    {
        return vbslq_f64(vcltq_f64(b, a), b, a);
    }
    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE max(const type a, const type b) noexcept
    {
        return vbslq_f64(vcltq_f64(a, b), b, a);
    }

    static mask_type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE is_nan(const type v) noexcept
    {
        const auto inf = score::cpp::bit_cast<std::uint64_t>(std::numeric_limits<double>::infinity());
        const auto abs_v = vandq_u64(vreinterpretq_u64_f64(v), vdupq_n_u64(0x7FFF'FFFF'FFFF'FFFFU));
        return vcltq_u64(vdupq_n_u64(inf), abs_v);
    }

    static type SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE blend(const type a,
                                                                       const type b,
                                                                       const mask_type c) noexcept
    {
        return vbslq_f64(c, b, a);
    }
};

template <typename T>
struct neon_abi;

template <>
struct neon_abi<std::int32_t>
{
    using impl = neon_backend<std::int32_t>;
    using mask_impl = neon_mask_backend<std::int32_t>;
};
template <>
struct neon_abi<float>
{
    using impl = neon_backend<float>;
    using mask_impl = neon_mask_backend<float>;
};
template <>
struct neon_abi<double>
{
    using impl = neon_backend<double>;
    using mask_impl = neon_mask_backend<double>;
};

template <typename T, std::size_t N>
struct neon_array_abi;

template <>
struct neon_array_abi<float, 16>
{
    using impl = array<float, neon_backend<float>, neon_mask_backend<float>, 0, 1, 2, 3>;
    using mask_impl = array_mask<float, neon_mask_backend<float>, 0, 1, 2, 3>;
};

template <>
struct native_abi<std::int32_t>
{
    using type = neon_abi<std::int32_t>;
};
template <>
struct native_abi<float>
{
    using type = neon_abi<float>;
};
template <>
struct native_abi<double>
{
    using type = neon_abi<double>;
};

template <>
struct deduce_abi<std::int32_t, 4>
{
    using type = native_abi<std::int32_t>::type;
};
template <>
struct deduce_abi<float, 4>
{
    using type = native_abi<float>::type;
};
template <>
struct deduce_abi<float, 16>
{
    using type = neon_array_abi<float, 16>;
};
template <>
struct deduce_abi<double, 2>
{
    using type = native_abi<double>::type;
};

} // namespace detail

template <>
struct is_abi_tag<detail::neon_abi<std::int32_t>> : std::true_type
{
};
template <>
struct is_abi_tag<detail::neon_abi<float>> : std::true_type
{
};
template <>
struct is_abi_tag<detail::neon_array_abi<float, 16>> : std::true_type
{
};
template <>
struct is_abi_tag<detail::neon_abi<double>> : std::true_type
{
};

} // namespace simd
} // namespace score::cpp

#undef SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_SIMD_AARCH64_NEON_ALWAYS_INLINE

#endif // SCORE_LANGUAGE_FUTURECPP_PRIVATE_SIMD_AARCH64_NEON_BACKEND_HPP
