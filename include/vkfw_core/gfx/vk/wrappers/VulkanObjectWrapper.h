/**
 * @file   VulkanObjectWrapper.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.09.20
 *
 * @brief  Base class for vulkan object wrappers.
 */

#pragma once

namespace vkfw_core::gfx {

    template<typename T>
    class VulkanObjectWrapper
    {
    public:
        VulkanObjectWrapper(T handle) : m_handle{handle} {}

        T GetHandle() const { return m_handle; }

    private:
        T m_handle;
    };
}
