/**
 * @file   Resource.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2013.12.31
 *
 * @brief  Contains Resource, a base class for all managed resources.
 */

#pragma once

#include "main.h"

namespace vku::gfx {
    class LogicalDevice;
}

namespace vku {

    struct file_not_found {
        std::string filename_;
    };

    /**
     * @brief  Base class for all managed resources.
     *
     * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
     * @date   2013.12.31
     */
    class Resource
    {
    public:
        Resource(const std::string& resourceId, const gfx::LogicalDevice* device);
        Resource(const Resource&);
        Resource& operator=(const Resource&);
        Resource(Resource&&) noexcept;
        Resource& operator=(Resource&&) noexcept;
        virtual ~Resource();

        const std::string& getId() const;

    protected:
        const gfx::LogicalDevice* GetDevice() const { return device_; }
        std::string FindResourceLocation(const std::string& localFilename) const;

    private:
        /** Holds the resources id. */
        std::string id_;
        /** Holds the device object for dependencies. */
        const gfx::LogicalDevice* device_;
    };
}
