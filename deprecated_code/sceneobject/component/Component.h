/**
 * @file   Component.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.04.09
 *
 * @brief  Generic representation of a Component in the Engine.
 */

#pragma once

#include "main.h"

namespace vku {

    /// Maximum number of different component types.
    constexpr std::uint32_t MAX_COMPONENT_FAMILIES = 32;

    ///
    /// Base component for the actual component.
    /// This class is only there to have make the family counter for all
    /// component - types work.
    /// This enables the functionality to call Family() on all Component<T> types
    /// and get the following:
    /// Component<A>::Family() -> 0
    /// Component<B>::Family() -> 1
    /// Component<A>::Family() -> 0
    /// Component<C>::Family() -> 2
    ///
    class BaseComponent
    {
    protected:
        static std::uint32_t familyCounter_;
    };

    ///
    /// The templated core Component that can be used to define new components for
    /// GameObjects.
    ///
    template <typename T> class Component : public BaseComponent
    {
    public:
        static std::uint32_t Family();
        std::uint32_t GetGameObjectHandle() const noexcept { return sceneObjectHandle_; }
        void SetGameObjectHandle(std::uint32_t handle) noexcept { sceneObjectHandle_ = handle; }

    protected:
        std::uint32_t sceneObjectHandle_;
    };

    ///
    /// Returns the 'family' of a type of Component
    /// \return family of the component
    ///
    template <typename T> std::uint32_t Component<T>::Family()
    {
        // This is only executed once of all types of Component (Component<A>,
        // Component<B>, ...), so family will be the same for all instance of one
        // types of component.
        // This also needs to be exactly this way, because this ensures, the
        // family_counter_ is only incremented once for every types of component.
        static std::uint32_t family = familyCounter_++;
        assert(familyCounter_ < MAX_COMPONENT_FAMILIES);
        return family;
    }
}
