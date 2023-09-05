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
    concept Material = requires(const T a, std::span<std::uint8_t>& b, std::uint32_t c)
    {
        T::MATERIAL_ID;
        { T::MATERIAL_ID } -> std::convertible_to<std::uint32_t>;
        T::GetGPUSize();
        { T::GetGPUSize() } -> std::convertible_to<std::size_t>;
        T::FillGPUInfo(a, b, c);
    };

    template<typename T>
    concept VulkanObject = requires
    {
        typename T::CType;
    };

    template<typename T>
    concept UniqueVulkanObject = requires
    {
        typename T::element_type;
        typename T::element_type::CType;
    };
}
