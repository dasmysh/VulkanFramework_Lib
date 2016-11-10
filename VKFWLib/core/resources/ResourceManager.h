/**
 * @file   ResourceManager.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2014.01.03
 *
 * @brief  Contains the base class for all resource managers.
 */

#pragma once

#include <boost/exception/all.hpp>

namespace vku {
    namespace gfx {
        class LogicalDevice;
    }

    class ApplicationBase;

    using errdesc_info = boost::error_info<struct tag_errdesc, std::string>;
    using resid_info = boost::error_info<struct tag_resid, std::string>;

    /**
     * Exception base class for resource loading errors.
     */
    struct resource_loading_error : virtual boost::exception, virtual std::exception { };

    template<typename rType>
    struct DefaultResourceLoadingPolicy
    {
        static std::shared_ptr<rType> CreateResource(const std::string& resId, const gfx::LogicalDevice* device)
        {
            return std::move(std::make_shared<rType>(resId, device));
        }
    };

    /**
     * @brief  Base class for all resource managers.
     *
     * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
     * @date   2014.01.03
     */
    template<typename rType, bool reloadLoop = false, typename ResourceLoadingPolicy = DefaultResourceLoadingPolicy<rType>>
    class ResourceManager
    {
    protected:
        /** The resource managers resource type. */
        using ResourceType = rType;
        /** The resource map type. */
        using ResourceMap = std::unordered_map<std::string, std::weak_ptr<rType>>;
        /** The type of this base class. */
        using ResourceManagerBase = ResourceManager<rType, reloadLoop, ResourceLoadingPolicy>;
        /** The resource loading policy used. */
        using LoadingPolicy = ResourceLoadingPolicy;

    public:
        /** Constructor for resource managers. */
        explicit ResourceManager(const gfx::LogicalDevice* device) : device_{ device } {}

        /** Copy constructor. */
        ResourceManager(const ResourceManager& rhs) :
            device_{ rhs.device_ }
        {
            for (const auto& res : rhs.resources) {
                resources.emplace(res.first, std::weak_ptr<ResourceType>());
            }
        }

        /** Copy assignment operator. */
        ResourceManager& operator=(const ResourceManager& rhs)
        {
            if (this != &rhs) {
                ResourceManager tmp{ rhs };
                std::swap(*this, tmp);
            }
            return *this;
        }

        /** Move constructor. */
        ResourceManager(ResourceManager&& rhs) noexcept : resources{ std::move(rhs.resources) }, device_{ rhs.device_ } {}
        /** Move assignment operator. */
        ResourceManager& operator=(ResourceManager&& rhs) noexcept
        {
            if (this != &rhs) {
                this->~ResourceManager();
                resources = std::move(rhs.resources);
                device_ = rhs.device_;
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
        std::shared_ptr<ResourceType> GetResource(const std::string& resId)
        {
            std::weak_ptr<ResourceType> wpResource;
            try {
                wpResource = resources.at(resId);
            }
            catch (std::out_of_range e) {
                LOG(INFO) << "No resource with id \"" << resId << "\" found. Creating new one.";
            }
            if (wpResource.expired()) {
                std::shared_ptr<ResourceType> spResource(nullptr);
                LoadResource(resId, spResource);
                while (reloadLoop && !spResource) {
                    LoadResource(resId, spResource);
                }
                wpResource = spResource;
                resources.insert(std::move(std::make_pair(resId, wpResource)));
                return std::move(spResource);
            }
            return wpResource.lock();
        }

        /**
         * Checks if the resource manager contains this resource.
         * @param resId the resources id
         * @return whether the manager contains the resource or not.
         */
        bool HasResource(const std::string& resId) const
        {
            return (resources.find(resId) != resources.end());
        }


    protected:
        /**
         * Loads a new resource and handles errors.
         * @param resId the resource id to load.
         * @param spResource pointer to the resource to fill.
         */
        virtual void LoadResource(const std::string& resId, std::shared_ptr<ResourceType>& spResource)
        {
            try {
                spResource = std::move(LoadingPolicy::CreateResource(TranslateCreationParameters(resId), device_));
            }
            catch (const resource_loading_error& loadingError) {
                auto resid = boost::get_error_info<resid_info>(loadingError);
                auto filename = boost::get_error_info<boost::errinfo_file_name>(loadingError);
                auto errDesc = boost::get_error_info<errdesc_info>(loadingError);
                LOG(INFO) << "Error while loading resource \"" << resId << "\"/\"" << resId.c_str() << "\"." << std::endl
                    << "ResourceID: " << (resid == nullptr ? "-" : resid->c_str()) << std::endl
                    << "Filename: " << (filename == nullptr ? "-" : filename->c_str()) << std::endl
                    << "Description: " << (errDesc == nullptr ? "-" : errDesc->c_str());
                if (!reloadLoop) throw;
            }
        }

        virtual std::string TranslateCreationParameters(const std::string& id)
        {
            return id;
        }

        /**
         *  Sets the resource with a given name to a new value.
         *  @param resourceName the name of the resource.
         *  @param resource the new resource.
         *  @return a pointer to the new resource.
         */
        std::shared_ptr<ResourceType> SetResource(const std::string& resourceName, std::shared_ptr<ResourceType>&& resource)
        {
            resources[resourceName] = std::move(resource);
            return resources[resourceName].lock();
        }

        /** Holds the resources managed. */
        ResourceMap resources;
        /** Holds the device for this resource. */
        const gfx::LogicalDevice* device_;
    };
}
