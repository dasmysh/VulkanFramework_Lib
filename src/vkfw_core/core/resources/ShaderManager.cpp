/**
 * @file   ShaderManager.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2014.01.15
 *
 * @brief  Contains the implementation of ShaderManager.
 */

#include "core/resources/ShaderManager.h"

namespace vku {
    /**
     * Constructor.
     * @param device the device to create resources in this manager.
     */
    ShaderManager::ShaderManager(const gfx::LogicalDevice* device) :
        ResourceManager(device)
    {
    }

    /** Default copy constructor. */
    ShaderManager::ShaderManager(const ShaderManager&) = default;
    /** Default copy assignment operator. */
    ShaderManager& ShaderManager::operator=(const ShaderManager&) = default;

    /** Default move constructor. */
    ShaderManager::ShaderManager(ShaderManager&& rhs) noexcept : ResourceManagerBase(std::move(rhs)) {}

    /** Default move assignment operator. */
    ShaderManager& ShaderManager::operator=(ShaderManager&& rhs) noexcept
    {
        ResourceManagerBase* tResMan = this;
        *tResMan = static_cast<ResourceManagerBase&&>(std::move(rhs));
        return *this;
    }

    /** Default destructor. */
    ShaderManager::~ShaderManager() = default;
}
