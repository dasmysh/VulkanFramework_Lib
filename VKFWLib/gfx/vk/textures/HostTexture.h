/**
 * @file   HostTexture.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2017.02.20
 *
 * @brief  Declaration of a general host accessible Vulkan image.
 */

#pragma once

#include "main.h"
#include "Texture.h"
#include "core/type_traits.h"

namespace vku { namespace gfx {

    class HostTexture final : public Texture
    {
    public:
        HostTexture(const LogicalDevice* device, uint32_t width, uint32_t height, uint32_t depth,
            const TextureDescriptor& desc,
            const std::vector<uint32_t>& queueFamilyIndices = std::vector<uint32_t>{});
        ~HostTexture();
        HostTexture(const HostTexture&);
        HostTexture& operator=(const HostTexture&);
        HostTexture(HostTexture&&) noexcept;
        HostTexture& operator=(HostTexture&&) noexcept;

        void InitializeData(size_t bufferSize, size_t dataSize, const void* data);
        void InitializeData(size_t size, const void* data);
        void UploadData(size_t offset, size_t size, const void* data);
        void DownloadData(size_t size, void* data) const;

        template<class T> std::enable_if_t<vku::has_contiguous_memory<T>::value> InitializeData(size_t bufferSize, const T& data);
        template<class T> std::enable_if_t<vku::has_contiguous_memory<T>::value> InitializeData(const T& data);
        template<class T> std::enable_if_t<vku::has_contiguous_memory<T>::value> UploadData(size_t offset, const T& data);
        template<class T> std::enable_if_t<vku::has_contiguous_memory<T>::value>  DownloadData(T& data) const;

    private:
        void UploadDataInternal(size_t offset, size_t size, const void* data) const;
    };

    template <class T> std::enable_if_t<vku::has_contiguous_memory<T>::value> HostTexture::InitializeData(size_t bufferSize, const T& data)
    {
        InitializeData(bufferSize, byteSizeOf(data), data.data());
    }

    template <class T> std::enable_if_t<vku::has_contiguous_memory<T>::value> HostTexture::InitializeData(const T& data)
    {
        InitializeData(byteSizeOf(data), data.data());
    }

    template <class T> std::enable_if_t<vku::has_contiguous_memory<T>::value> HostTexture::UploadData(size_t offset, const T& data)
    {
        UploadData(offset, byteSizeOf(data), data.data());
    }

    template <class T> std::enable_if_t<vku::has_contiguous_memory<T>::value> HostTexture::DownloadData(T& data) const
    {
        DownloadData(byteSizeOf(data), data.data());
    }
}}
