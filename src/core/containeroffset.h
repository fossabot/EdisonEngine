#pragma once

#include "tpl_helper.h"

#include <vector>

namespace core
{
template<typename OffsetType, typename... DataTypes>
struct ContainerOffset
{
    static_assert(std::is_integral<OffsetType>::value && !std::is_signed<OffsetType>::value,
                  "Index type must be unsigned integer like");
    static_assert(sizeof...(DataTypes) > 0, "Must provide at least one bound type");

    using offset_type = OffsetType;

    offset_type offset = 0;

    constexpr ContainerOffset() = default;

    constexpr ContainerOffset(offset_type offset)
        : offset{offset}
    {
    }

    template<typename T>
    explicit ContainerOffset(T) = delete;

    template<typename T>
    offset_type index() const
    {
        static_assert(tpl::contains_v<T, DataTypes...>, "Can only use declared types for index conversion");
        if(offset % sizeof(T) != 0)
            throw std::runtime_error("Offset not dividable by element size");

        return offset / sizeof(T);
    }

    template<typename T>
    constexpr std::enable_if_t<tpl::contains_v<T, DataTypes...>, T&> from(std::vector<T>& v) const
    {
        if(offset % sizeof(T) != 0)
            throw std::runtime_error("Offset not dividable by element size");

        return v[offset / sizeof(T)];
    }

    template<typename T>
    constexpr std::enable_if_t<tpl::contains_v<T, DataTypes...>, T&> checkedFrom(std::vector<T>& v) const
    {
        if(offset % sizeof(T) != 0)
            throw std::runtime_error("Offset not dividable by element size");

        return v.at(offset / sizeof(T));
    }

    template<typename T>
    constexpr std::enable_if_t<tpl::contains_v<T, DataTypes...>, const T&>

        from(const std::vector<T>& v) const
    {
        if(offset % sizeof(T) != 0)
            throw std::runtime_error("Offset not dividable by element size");

        return v[offset / sizeof(T)];
    }

    template<typename T>
    constexpr std::enable_if_t<tpl::contains_v<T, DataTypes...>, const T&>

        checkedFrom(const std::vector<T>& v) const
    {
        if(offset % sizeof(T) != 0)
            throw std::runtime_error("Offset not dividable by element size");

        return v.at(offset / sizeof(T));
    }
};

template<typename IndexType, typename... DataTypes>
struct ContainerIndex
{
    static_assert(std::is_integral<IndexType>::value && !std::is_signed<IndexType>::value,
                  "Index type must be unsigned integer like");
    static_assert(sizeof...(DataTypes) > 0, "Must provide at least one bound type");

    using index_type = IndexType;

    index_type index = 0;

    constexpr ContainerIndex() = default;

    constexpr ContainerIndex(index_type index) noexcept
        : index{index}
    {
    }

    template<typename T>
    explicit ContainerIndex(T) = delete;

    template<typename T>
    constexpr std::enable_if_t<tpl::contains_v<T, DataTypes...>, T&> from(std::vector<T>& v) const
    {
        return v[index];
    }

    template<typename T>
    constexpr std::enable_if_t<tpl::contains_v<T, DataTypes...>, T&> checkedFrom(std::vector<T>& v) const
    {
        return v[index];
    }

    template<typename T>
    constexpr std::enable_if_t<tpl::contains_v<T, DataTypes...>, const T&>

        from(const std::vector<T>& v) const
    {
        return v.at(index);
    }

    template<typename T>
    constexpr std::enable_if_t<tpl::contains_v<T, DataTypes...>, const T&>

        checkedFrom(const std::vector<T>& v) const
    {
        return v.at(index);
    }

    auto& operator+=(index_type delta)
    {
        if((index > 0) && (delta > std::numeric_limits<index_type>::max() - index))
            throw std::out_of_range("Index addition causes overflow");

        index += delta;
        return *this;
    }

    auto operator+(const ContainerIndex<IndexType, DataTypes...>& delta) const
    {
        if((index > 0) && (delta.index > std::numeric_limits<index_type>::max() - index))
            throw std::out_of_range("Index addition causes overflow");

        return index + delta.index;
    }

    template<typename T>
    void operator+=(T) = delete;
};
}