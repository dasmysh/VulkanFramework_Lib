/**
 * @file   ComponentManager.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.04.10
 *
 * @brief  Implementations for the BaseComponentManager.
 */

#include "ComponentManager.h"

namespace vku {

    BaseComponentManager::~BaseComponentManager() = default;

    ///
    /// Returns a handle to a free slot and removes the slot from the free-list.
    ///
    /// \return Handle to free slot, or an INVAlID_HANDLE, if there is no free
    /// slot left.
    ///
    std::uint32_t BaseComponentManager::PopFreeSlot() noexcept
    {
        if (HasFreeSlots()) {
            std::uint32_t to_return = freeSlots_.back();
            freeSlots_.pop_back();
            return to_return;
        }
        return std::numeric_limits<std::uint32_t>::max();
    }
}
