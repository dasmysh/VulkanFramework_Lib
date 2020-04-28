/**
 * @file   Resource.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2013.12.31
 *
 * @brief  Contains Resource, a base class for all managed resources.
 */

#pragma once

#include "main.h"

namespace vkfw_core::gfx {
    class LogicalDevice;
}

namespace vkfw_core {

    struct file_not_found final : public std::exception {
        explicit file_not_found(std::string filename) : std::exception{ "File not found." }, m_filename{ std::move(filename) } {}
        std::string m_filename;
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
        Resource(std::string resourceId, const gfx::LogicalDevice* device);
        Resource(const Resource&);
        Resource& operator=(const Resource&);
        Resource(Resource&&) noexcept;
        Resource& operator=(Resource&&) noexcept;
        virtual ~Resource();

        [[nodiscard]] const std::string& GetId() const;

        static std::string FindGeneralFileLocation(const std::string& localFilename, const std::string& resourceId = "_no_resource_");

    protected:
        [[nodiscard]] const gfx::LogicalDevice* GetDevice() const { return m_device; }
        [[nodiscard]] std::string FindResourceLocation(const std::string& localFilename) const;

    private:
        /** Holds the resources id. */
        std::string m_id;
        /** Holds the device object for dependencies. */
        const gfx::LogicalDevice* m_device = nullptr;
    };
}
