/**
 * @file   DescriptorSetLayout.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2020.12.01
 *
 * @brief  Declaration of a helper class to handle descriptor layouts.
 */

#pragma once

#include "main.h"
#include "gfx/vk/wrappers/VulkanObjectWrapper.h"
#include "gfx/vk/wrappers/DescriptorPool.h"
#include "gfx/vk/wrappers/DescriptorSet.h"

namespace vkfw_core::gfx {
    class LogicalDevice;
    class Texture;
    class Buffer;
}

namespace vkfw_core::gfx {

    /** This class is based strongly on https://github.com/nvpro-samples/shared_sources/blob/master/nvvk/descriptorsets_vk.hpp */
    class DescriptorSetLayout : public VulkanObjectWrapper<vk::UniqueDescriptorSetLayout>
    {
    public:
        DescriptorSetLayout(std::string_view name);
        DescriptorSetLayout(DescriptorSetLayout&& rhs) noexcept;
        DescriptorSetLayout& operator=(DescriptorSetLayout&& rhs) noexcept;
        ~DescriptorSetLayout();

        void AddBinding(std::uint32_t binding, vk::DescriptorType type, std::uint32_t count,
                        vk::ShaderStageFlags stageFlags, const vk::Sampler* sampler = nullptr);

        vk::DescriptorSetLayout CreateDescriptorLayout(const LogicalDevice* device);
        [[nodiscard]] DescriptorPool CreateDescriptorPool(const LogicalDevice* device, std::string_view name);

        void AddDescriptorPoolSizes(std::vector<vk::DescriptorPoolSize>& poolSizes, std::size_t setCount) const;
        [[nodiscard]] static DescriptorPool CreateDescriptorPool(const LogicalDevice* device, std::string_view name,
                                                                 const std::vector<vk::DescriptorPoolSize>& poolSizes,
                                                                 std::size_t setCount);

        [[nodiscard]] const auto& GetBindings() const { return m_bindings; }
        [[nodiscard]] static bool IsBufferBindingType(vk::DescriptorType type);
        [[nodiscard]] static bool IsDynamicBindingType(vk::DescriptorType type);
        [[nodiscard]] static bool IsTexelBufferBindingType(vk::DescriptorType type);
        [[nodiscard]] static bool IsImageBindingType(vk::DescriptorType type);

    private:

        /** The bindings of the descriptor set. */
        std::vector<vk::DescriptorSetLayoutBinding> m_bindings;
    };
}
