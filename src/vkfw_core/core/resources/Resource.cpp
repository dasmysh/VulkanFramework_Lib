/**
* @file    Resource.cpp
* @author  Sebastian Maisch <sebastian.maisch@googlemail.com>
* @date    2014.03.29
*
* @brief   Implementations for the resource class.
*/

#include "core/resources/Resource.h"
#include <filesystem>
#include "app/ApplicationBase.h"
#include "app/Configuration.h"

namespace vkfw_core {
    /**
     * Constructor.
     * @param resourceId the resource id to use
     */
    Resource::Resource(std::string resourceId, const gfx::LogicalDevice* device)
        : id_{std::move(resourceId)}, device_{device}
    {
    }

    /** Default copy constructor. */
    Resource::Resource(const Resource&) = default;

    /** Default assignment operator. */
    Resource& Resource::operator=(const Resource&) = default;

    /** Move constructor. */
    Resource::Resource(Resource&& orig) noexcept : id_{std::move(orig.id_)}, device_{orig.device_} {};

    /** Move assignment operator. */
    Resource& Resource::operator=(Resource&& orig) noexcept
    {
        if (this != &orig) {
            this->~Resource();
            id_ = std::move(orig.id_);
            device_ = orig.device_;
        }
        return *this;
    };

    Resource::~Resource() = default;

    /**
    * Accessor to the resources id.
    * @return the resources id
    */
    const std::string& Resource::getId() const
    {
        return id_;
    };

    /**
     *  Returns the actual location of the resource by looking into all search paths.
     *  @param localFilename the file name local to any resource base directory.
     *  @param resourceId the id of the resource to look for for error logging.
     *  @return the path to the resource.
     */
    std::string Resource::FindGeneralFileLocation(const std::string& localFilename, const std::string& resourceId)
    {
        auto filename = ApplicationBase::instance().GetConfig().resourceBase_ + "/" + localFilename;
        if (std::filesystem::exists(filename)) { return filename; }

        for (const auto& dir : ApplicationBase::instance().GetConfig().resourceDirs_) {
            filename = dir;
            filename.append("/").append(localFilename);
            if (dir.empty()) { filename = localFilename; }
            if (std::filesystem::exists(filename)) { return filename; }
        }

        spdlog::error("Error while loading resource.\nResourceID: {}\nFilename: {}\nDescription: Cannot find local resource file.", resourceId, localFilename);

        throw file_not_found{ localFilename };
    }

    /**
     *  Returns the actual location of the resource by looking into all search paths.
     *  @param localFilename the file name local to any resource base directory.
     *  @return the path to the resource.
     */
    std::string Resource::FindResourceLocation(const std::string& localFilename) const
    {
        return Resource::FindGeneralFileLocation(localFilename, id_);
    }
}
