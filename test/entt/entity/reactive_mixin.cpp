#include <array>
#include <memory>
#include <type_traits>
#include <utility>
#include <gtest/gtest.h>
#include <entt/core/type_info.hpp>
#include <entt/entity/component.hpp>
#include <entt/entity/mixin.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/storage.hpp>
#include "../../common/empty.h"
#include "../../common/linter.hpp"
template<typename Type>
struct ReactiveMixin: testing::Test {
    using type = Type;
};

template<typename Type>
using ReactiveMixinDeathTest = ReactiveMixin<Type>;

using ReactiveMixinTypes = ::testing::Types<void, bool>;

TYPED_TEST_SUITE(ReactiveMixin, ReactiveMixinTypes, );
TYPED_TEST_SUITE(ReactiveMixinDeathTest, ReactiveMixinTypes, );

TYPED_TEST(ReactiveMixin, Constructors) {
    using value_type = typename TestFixture::type;
    using traits_type = entt::component_traits<value_type>;

    entt::reactive_mixin<entt::storage<value_type>> pool;

    ASSERT_EQ(pool.policy(), entt::deletion_policy{traits_type::in_place_delete});
    ASSERT_NO_THROW([[maybe_unused]] auto alloc = pool.get_allocator());
    ASSERT_EQ(pool.type(), entt::type_id<value_type>());

    pool = entt::reactive_mixin<entt::storage<value_type>>{std::allocator<value_type>{}};

    ASSERT_EQ(pool.policy(), entt::deletion_policy{traits_type::in_place_delete});
    ASSERT_NO_THROW([[maybe_unused]] auto alloc = pool.get_allocator());
    ASSERT_EQ(pool.type(), entt::type_id<value_type>());
}

TYPED_TEST(ReactiveMixin, Move) {
    using value_type = typename TestFixture::type;

    entt::registry registry;
    entt::reactive_mixin<entt::storage<value_type>> pool;
    const std::array entity{registry.create(), registry.create()};

    pool.bind(registry);
    pool.template on_construct<test::empty>();
    pool.template on_update<test::empty>();
    registry.emplace<test::empty>(entity[0u]);

    static_assert(std::is_move_constructible_v<decltype(pool)>, "Move constructible type required");
    static_assert(std::is_move_assignable_v<decltype(pool)>, "Move assignable type required");

    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_EQ(pool.type(), entt::type_id<value_type>());

    entt::reactive_mixin<entt::storage<value_type>> other{std::move(pool)};

    test::is_initialized(pool);

    ASSERT_TRUE(pool.empty());
    ASSERT_FALSE(other.empty());

    ASSERT_EQ(other.type(), entt::type_id<value_type>());

    ASSERT_EQ(other.index(entity[0u]), 0u);
    ASSERT_EQ(&other.registry(), &registry);

    other.clear();
    registry.replace<test::empty>(entity[0u]);

    ASSERT_FALSE(pool.empty());
    ASSERT_TRUE(other.empty());

    std::swap(other, pool);
    pool = std::move(other);
    test::is_initialized(other);

    ASSERT_FALSE(pool.empty());
    ASSERT_TRUE(other.empty());

    ASSERT_EQ(pool.index(entity[0u]), 0u);
    ASSERT_EQ(&pool.registry(), &registry);

    other = entt::reactive_mixin<entt::storage<value_type>>{};
    other.bind(registry);
    other.template on_construct<test::empty>();
    registry.on_construct<test::empty>().disconnect(&pool);

    registry.emplace<test::empty>(entity[1u]);
    other = std::move(pool);
    test::is_initialized(pool);

    ASSERT_FALSE(pool.empty());
    ASSERT_FALSE(other.empty());

    ASSERT_EQ(other.index(entity[0u]), 0u);
    ASSERT_EQ(&other.registry(), &pool.registry());
}

