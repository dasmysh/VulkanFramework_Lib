/**
 * @file   concepts.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2020.11.22
 *
 * @brief  A collection of concepts used in the framework.
 */

#pragma once

#include "main.h"
#include <concepts>

namespace vkfw_core::gfx {
    class MeshInfo;
}

namespace vkfw_core {

    template<typename T> concept Vertex = requires
    {
        T::m_bindingDescription;
        { T::m_bindingDescription } -> std::convertible_to<vk::VertexInputBindingDescription>;
        T::m_attributeDescriptions;
        { T::m_attributeDescriptions[0] } -> std::convertible_to<vk::VertexInputAttributeDescription>;
        std::constructible_from<T, const vkfw_core::gfx::MeshInfo*, std::size_t>;
    };

    template<typename T>
    concept VulkanObject = requires
    {
        T::CType;
        { T::CType } -> std::convertible_to<void*>;
    };
}
