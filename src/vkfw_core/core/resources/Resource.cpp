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
        : m_id{std::move(resourceId)}, m_device{device}
    {
    }

    /** Default copy constructor. */
    Resource::Resource(const Resource&) = default;

    /** Default assignment operator. */
    Resource& Resource::operator=(const Resource&) = default;

    /** Move constructor. */
    Resource::Resource(Resource&& orig) noexcept : m_id{std::move(orig.m_id)}, m_device{orig.m_device} {};

    /** Move assignment operator. */
    Resource& Resource::operator=(Resource&& orig) noexcept
    {
        if (this != &orig) {
            this->~Resource();
            m_id = std::move(orig.m_id);
            m_device = orig.m_device;
        }
        return *this;
    };

    Resource::~Resource() = default;

    /**
    * Accessor to the resources id.
    * @return the resources id
    */
    const std::string& Resource::GetId() const
    {
        return m_id;
    };

    /**
     *  Returns the actual location of the resource by looking into all search paths.
     *  @param localFilename the file name local to any resource base directory.
     *  @param resourceId the id of the resource to look for for error logging.
     *  @return the path to the resource.
     */
    std::string Resource::FindGeneralFileLocation(const std::string& localFilename, const std::string& resourceId)
    {
        auto filename = ApplicationBase::instance().GetConfig().m_resourceBase + "/" + localFilename;
        if (std::filesystem::exists(filename)) { return filename; }

        for (const auto& dir : ApplicationBase::instance().GetConfig().m_resourceDirs) {
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
        return Resource::FindGeneralFileLocation(localFilename, m_id);
    }
}
