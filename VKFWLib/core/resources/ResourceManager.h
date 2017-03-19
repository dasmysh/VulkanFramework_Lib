/**
 * @file   ResourceManager.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2014.01.03
 *
 * @brief  Contains the base class for all resource managers.
 */

#pragma once

namespace vku::gfx {
    class LogicalDevice;
}

namespace vku {

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
        explicit ResourceManager(const gfx::LogicalDevice* device) : device_{ device } {}

        /** Copy constructor. */
        ResourceManager(const ResourceManager& rhs) :
            device_{ rhs.device_ }
        {
            for (const auto& res : rhs.resources_) {
                resources_.emplace(res.first, std::weak_ptr<ResourceType>());
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
        ResourceManager(ResourceManager&& rhs) noexcept : resources_{ std::move(rhs.resources_) }, device_{ rhs.device_ } {}
        /** Move assignment operator. */
        ResourceManager& operator=(ResourceManager&& rhs) noexcept
        {
            if (this != &rhs) {
                this->~ResourceManager();
                resources_ = std::move(rhs.resources_);
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
        template<typename... Args>
        std::shared_ptr<ResourceType> GetResource(const std::string& resId, Args&&... args)
        {
            std::weak_ptr<ResourceType> wpResource;
            try {
                wpResource = resources_.at(resId);
            }
            catch (std::out_of_range e) {
                LOG(INFO) << "No resource with id \"" << resId << "\" found. Creating new one.";
            }
            if (wpResource.expired()) {
                std::shared_ptr<ResourceType> spResource(nullptr);
                LoadResource(resId, spResource, std::forward<Args>(args)...);
                while (reloadLoop && !spResource) {
                    LoadResource(resId, spResource);
                }
                wpResource = spResource;
                resources_.insert(std::move(std::make_pair(resId, wpResource)));
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
            auto rit = resources_.find(resId);
            return (rit != resources_.end()) && !rit->expired();
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
            spResource = std::make_shared<rType>(resId, device_, std::forward<Args>(args)...);
            
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
            resources_[resourceName] = std::move(resource);
            return resources_[resourceName].lock();
        }

        /** Holds the resources managed. */
        ResourceMap resources_;
        /** Holds the device for this resource. */
        const gfx::LogicalDevice* device_;
    };
}
