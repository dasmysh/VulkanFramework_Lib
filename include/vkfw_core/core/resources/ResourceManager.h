/**
 * @file   ResourceManager.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2014.01.03
 *
 * @brief  Contains the base class for all resource managers.
 */

#pragma once

#include <spdlog/spdlog.h>

namespace vkfw_core::gfx {
    class LogicalDevice;
}

namespace vkfw_core {

    class ApplicationBase;

    /**
     * @brief  Base class for all resource managers.
     *
     * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
     * @date   2014.01.03
     */
    template<typename rType, bool reloadLoop = false>//, typename ResourceLoadingPolicy = DefaultResourceLoadingPolicy<rType>>
    class ResourceManager
    {
    protected:
        /** The resource managers resource type. */
        using ResourceType = rType;
        /** The resource map type. */
        using ResourceMap = std::unordered_map<std::string, std::weak_ptr<rType>>;
        /** The type of this base class. */
        using ResourceManagerBase = ResourceManager<rType, reloadLoop>;

    public:
        /** Constructor for resource managers. */
        explicit ResourceManager(const gfx::LogicalDevice* device) : m_device{ device } {}

        /** Copy constructor. */
        ResourceManager(const ResourceManager& rhs) : m_device{rhs.m_device}
        {
            for (const auto& res : rhs.m_resources) {
                m_resources.emplace(res.first, std::weak_ptr<ResourceType>());
            }
        }

        /** Copy assignment operator. */
        // NOLINTNEXTLINE
        ResourceManager& operator=(const ResourceManager& rhs)
        {
            if (this == &rhs) { return *this; }

            m_resources = rhs.m_resources;
            m_device = rhs.m_device;
            return *this;
        }

        /** Move constructor. */
        ResourceManager(ResourceManager&& rhs) noexcept
            : m_resources{std::move(rhs.m_resources)}, m_device{rhs.m_device}
        {
        }
        /** Move assignment operator. */
        ResourceManager& operator=(ResourceManager&& rhs) noexcept
        {
            if (this != &rhs) {
                this->~ResourceManager();
                m_resources = std::move(rhs.m_resources);
                m_device = rhs.m_device;
            }
            return *this;
        }
        /** Default destructor. */
        virtual ~ResourceManager() = default;

        /**
         * Gets a resource from the manager.
         * @param resId the resources id
         * @return the resource as a shared pointer
         */
        template<typename... Args>
        std::shared_ptr<ResourceType> GetResource(const std::string& resId, Args&&... args)
        {
            std::weak_ptr<ResourceType> wpResource;
            try {
                wpResource = m_resources.at(resId);
            }
            catch (std::out_of_range&) {
                spdlog::info("No resource with id \"{}\" found. Creating new one.", resId);
            }
            if (wpResource.expired()) {
                std::shared_ptr<ResourceType> spResource(nullptr);
                LoadResource(resId, spResource, std::forward<Args>(args)...);
                if constexpr (reloadLoop) { // NOLINT
                    while (!spResource) { LoadResource(resId, spResource, std::forward<Args>(args)...); }
                }
                wpResource = spResource;
                m_resources.insert(std::move(std::make_pair(resId, wpResource)));
                return std::move(spResource);
            }
            return wpResource.lock();
        }

        /**
         * Checks if the resource manager contains this resource.
         * @param resId the resources id
         * @return whether the manager contains the resource or not.
         */
        [[nodiscard]] bool HasResource(const std::string& resId) const
        {
            auto rit = m_resources.find(resId);
            return (rit != m_resources.end()) && !rit->expired();
        }


    protected:
        /**
         * Loads a new resource and handles errors.
         * @param resId the resource id to load.
         * @param spResource pointer to the resource to fill.
         */
        template<typename... Args>
        void LoadResource(const std::string& resId, std::shared_ptr<ResourceType>& spResource, Args&&... args)
        {
            spResource = std::make_shared<rType>(resId, m_device, std::forward<Args>(args)...);

            /*catch (const resource_loading_error& loadingError) {
                auto resid = boost::get_error_info<resid_info>(loadingError);
                auto filename = boost::get_error_info<boost::errinfo_file_name>(loadingError);
                auto errDesc = boost::get_error_info<errdesc_info>(loadingError);
                LOG(INFO) << "Error while loading resource \"" << resId << "\"/\"" << resId.c_str() << "\"." << std::endl
                    << "ResourceID: " << (resid == nullptr ? "-" : resid->c_str()) << std::endl
                    << "Filename: " << (filename == nullptr ? "-" : filename->c_str()) << std::endl
                    << "Description: " << (errDesc == nullptr ? "-" : errDesc->c_str());
                if (!reloadLoop) throw;
            }*/
        }

        /**
         *  Sets the resource with a given name to a new value.
         *  @param resourceName the name of the resource.
         *  @param resource the new resource.
         *  @return a pointer to the new resource.
         */
        std::shared_ptr<ResourceType> SetResource(const std::string& resourceName, std::shared_ptr<ResourceType>&& resource)
        {
            m_resources[resourceName] = std::move(resource);
            return m_resources[resourceName].lock();
        }

    private:
        /** Holds the resources managed. */
        ResourceMap m_resources;
        /** Holds the device for this resource. */
        const gfx::LogicalDevice* m_device;
    };
}
