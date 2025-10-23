///
/// \file
/// \copyright Copyright (C) 2020-2022, Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
///

#include <score/simd.hpp>
#include <score/simd.hpp> // test include guard

#include <score/assert_support.hpp>

#include <array>

#include <gtest/gtest.h>

namespace score::cpp
{
namespace
{

template <std::size_t N>
class generator
{
    static constexpr std::size_t size{N};

public:
    explicit generator(const bool v) : v_{} { v_.fill(v); }

    template <std::size_t M>
    constexpr bool operator()(std::integral_constant<std::size_t, M>) const noexcept
    {
        return std::get<std::integral_constant<std::size_t, M>{}()>(v_);
    }
    constexpr bool operator[](std::size_t i) const { return v_[i]; }
    constexpr bool& operator[](std::size_t i) { return v_[i]; }

private:
    std::array<bool, size> v_;
};

template <typename T>
class simd_mask_fixture : public testing::Test
{
};

using ElementTypes = ::testing::Types<simd_mask<std::int32_t>, simd_mask<float>, simd_mask<double>>;
TYPED_TEST_SUITE(simd_mask_fixture, ElementTypes, /*unused*/);

/// @testmethods TM_REQUIREMENT
/// @requirement CB-#18398051
TYPED_TEST(simd_mask_fixture, Broadcast)
{
    const TypeParam a{true};

    for (std::size_t i{0U}; i < a.size(); ++i)
    {
        EXPECT_TRUE(a[i]);
    }
}

/// @testmethods TM_REQUIREMENT
/// @requirement CB-#18398051
TYPED_TEST(simd_mask_fixture, Initialize)
{
    for (std::size_t i{0U}; i < TypeParam::size(); ++i)
    {
        generator<TypeParam::size()> gen{false};
        gen[i] = true;
        const TypeParam a{gen};

        for (std::size_t j{0U}; j < a.size(); ++j)
        {
            if (j == i)
            {
                EXPECT_TRUE(a[j]);
            }
            else
            {
                EXPECT_FALSE(a[j]);
            }
        }
    }
}

/// @testmethods TM_REQUIREMENT
/// @requirement CB-#18398051
TYPED_TEST(simd_mask_fixture, Access_WhenOutOfBounds_ThenPreconditionViolated)
{
    const TypeParam a{false};

    SCORE_LANGUAGE_FUTURECPP_EXPECT_CONTRACT_VIOLATED(a[a.size()]);
}

/// @testmethods TM_REQUIREMENT
/// @requirement CB-#18398051
TYPED_TEST(simd_mask_fixture, Not)
{
    {
        const TypeParam a{true};
        EXPECT_TRUE(none_of(!a));
    }
    {
        const TypeParam a{false};
        EXPECT_TRUE(all_of(!a));
    }
}

/// @testmethods TM_REQUIREMENT
/// @requirement CB-#18398051
TYPED_TEST(simd_mask_fixture, And)
{
    {
        const TypeParam a{true};
        EXPECT_TRUE(all_of(a && a));
    }
    {
        const TypeParam a{true};
        EXPECT_TRUE(none_of(a && !a));
    }
    {
        const TypeParam a{true};
        EXPECT_TRUE(none_of(!a && a));
    }
    {
        const TypeParam a{false};
        EXPECT_TRUE(none_of(a && a));
    }
}

/// @testmethods TM_REQUIREMENT
/// @requirement CB-#18398051
TYPED_TEST(simd_mask_fixture, Or)
{
    {
        const TypeParam a{true};
        EXPECT_TRUE(all_of(a || a));
    }
    {
        const TypeParam a{true};
        EXPECT_TRUE(all_of(a || !a));
    }
    {
        const TypeParam a{true};
        EXPECT_TRUE(all_of(!a || a));
    }
    {
        const TypeParam a{false};
        EXPECT_TRUE(none_of(a || a));
    }
}

/// @testmethods TM_REQUIREMENT
/// @requirement CB-#18398051
TYPED_TEST(simd_mask_fixture, AllOf_AllSet)
{
    const generator<TypeParam::size()> gen{true};
    const TypeParam a{gen};

    EXPECT_TRUE(all_of(a));
}

/// @testmethods TM_REQUIREMENT
/// @requirement CB-#18398051
TYPED_TEST(simd_mask_fixture, AllOf_NoneSet)
{
    const generator<TypeParam::size()> gen{false};
    const TypeParam a{gen};

    EXPECT_FALSE(all_of(a));
}

/// @testmethods TM_REQUIREMENT
/// @requirement CB-#18398051
TYPED_TEST(simd_mask_fixture, AllOf)
{
    generator<TypeParam::size()> gen{false};

    for (std::size_t i{0U}; i < TypeParam::size(); ++i)
    {
        gen[i] = true;
        const TypeParam a{gen};

        if (i == TypeParam::size() - 1U)
        {
            EXPECT_TRUE(all_of(a));
        }
        else
        {
            EXPECT_FALSE(all_of(a));
        }
    }
}

/// @testmethods TM_REQUIREMENT
/// @requirement CB-#18398051
TYPED_TEST(simd_mask_fixture, AnyOf_AllSet)
{
    const generator<TypeParam::size()> gen{true};
    const TypeParam a{gen};

    EXPECT_TRUE(all_of(a));
}

/// @testmethods TM_REQUIREMENT
/// @requirement CB-#18398051
TYPED_TEST(simd_mask_fixture, AnyOf_NoneSet)
{
    const generator<TypeParam::size()> gen{false};
    const TypeParam a{gen};

    EXPECT_FALSE(all_of(a));
}

/// @testmethods TM_REQUIREMENT
/// @requirement CB-#18398051
TYPED_TEST(simd_mask_fixture, AnyOf)
{
    for (std::size_t i{0U}; i < TypeParam::size(); ++i)
    {
        generator<TypeParam::size()> gen{false};
        gen[i] = true;
        const TypeParam a{gen};

        EXPECT_TRUE(any_of(a));
    }
}

/// @testmethods TM_REQUIREMENT
/// @requirement CB-#18398051
TYPED_TEST(simd_mask_fixture, NoneOf_AllSet)
{
    const generator<TypeParam::size()> gen{true};
    const TypeParam a{gen};

    EXPECT_FALSE(none_of(a));
}

/// @testmethods TM_REQUIREMENT
/// @requirement CB-#18398051
TYPED_TEST(simd_mask_fixture, NoneOf_NoneSet)
{
    const generator<TypeParam::size()> gen{false};
    const TypeParam a{gen};

    EXPECT_TRUE(none_of(a));
}

/// @testmethods TM_REQUIREMENT
/// @requirement CB-#18398051
TYPED_TEST(simd_mask_fixture, NoneOf)
{

    for (std::size_t i{0U}; i < TypeParam::size(); ++i)
    {
        generator<TypeParam::size()> gen{false};
        gen[i] = true;
        const TypeParam a{gen};

        EXPECT_FALSE(none_of(a));
    }
}

} // namespace
} // namespace score::cpp
