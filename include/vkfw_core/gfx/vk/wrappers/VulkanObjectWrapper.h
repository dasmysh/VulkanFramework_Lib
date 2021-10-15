/**
 * @file   VulkanObjectWrapper.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2021.09.20
 *
 * @brief  Base class for vulkan object wrappers.
 */

#pragma once

#include "core/concepts.h"

namespace vkfw_core::gfx {

    template<typename T>
    requires VulkanObject<T> || UniqueVulkanObject<T>
    struct VulkanObjectWrapperHelper
    {
        using BaseType = void;
    };

    template<VulkanObject T> struct VulkanObjectWrapperHelper<T>
    {
        using BaseType = T;
    };

    template<UniqueVulkanObject T> struct VulkanObjectWrapperHelper<T>
    {
        using BaseType = typename T::element_type;
    };

    template<typename T> class VulkanObjectPrivateWrapper
    {
    public:
        using BaseType = VulkanObjectWrapperHelper<T>::BaseType;
        using CType = BaseType::CType;

        VulkanObjectPrivateWrapper(vk::Device device, std::string_view name, T handle)
            : m_name{name}, m_handle{std::move(handle)}
        {
            CheckSetName(device);
        }

        const std::string& GetName() const { return m_name; }

        void SetHandle(vk::Device device, T handle)
        {
            assert(!m_handle && "Setting a handle is only allowed if initialized with nullptr.");
            m_handle = std::move(handle);
            CheckSetName(device);
        }

        void SetHandle(vk::Device device, std::string_view name, T handle)
        {
            m_name = name;
            SetHandle(device, handle);
        }

        operator bool() const { return GetHandle(); }

    protected:
        void CheckSetName(vk::Device device) const
        {
            if (device && m_handle) { SetObjectName(device, GetHandle(), m_name); }
        }

        BaseType GetHandle() const
        {
            if constexpr (UniqueVulkanObject<T>) {
                return *m_handle;
            } else {
                return m_handle;
            }
        }

        const BaseType* GetHandlePtr() const
        {
            if constexpr (UniqueVulkanObject<T>) {
                return &(*m_handle);
            } else {
                return &m_handle;
            }
        }

    private:
        static vk::ObjectType GetObjectType()
        {
            if constexpr (UniqueVulkanObject<T>) {
                return T::element_type::objectType;
            } else {
                return T::objectType;
            }
        }

#ifndef NDEBUG
        template<VulkanObject T> static void SetObjectName(vk::Device device, T object, std::string_view name)
        {
            device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT{
                GetObjectType(), reinterpret_cast<std::uint64_t>(static_cast<CType>(object)), name.data()});
        }
        template<VulkanObject T, typename Tag>
        static void SetObjectTag(vk::Device device, T object, std::uint64_t tagHandle, const Tag& tag)
        {
            device.setDebugUtilsObjectTagEXT(vk::DebugUtilsObjectTagInfoEXT{
                GetObjectType(), reinterpret_cast<std::uint64_t>(static_cast<CType>(object)), tagHandle, sizeof(Tag),
                &tag});
        }
#else
        template<VulkanObject T> static void SetObjectName(vk::Device, T, std::string_view) {}
        template<VulkanObject T, typename Tag> static void SetObjectTag(vk::Device, T, std::uint64_t, const Tag&) {}
#endif

        std::string m_name;
        T m_handle;
    };

    template<typename T> class VulkanObjectWrapper : public VulkanObjectPrivateWrapper<T>
    {
    public:
        using VulkanObjectPrivateWrapper<T>::VulkanObjectPrivateWrapper;
        using VulkanObjectPrivateWrapper<T>::GetHandle;
        using VulkanObjectPrivateWrapper<T>::GetHandlePtr;
    };
}
