/**
 * @file   enum_flags.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.03.23
 *
 * @brief  Implementation of a template for enum flags.
 */

#pragma once

#include <type_traits>

namespace vku {

    template<typename Enum> struct EnableBitMaskOperators { static constexpr bool enable = false; };
    template<typename Enum, typename Enable = void> class EnumFlags;

    template<typename Enum> class EnumFlags<Enum, typename std::enable_if<EnableBitMaskOperators<Enum>::enable>::type>
    {
    public:
        EnumFlags() : mask_(static_cast<typename std::underlying_type<Enum>::type>(0)) {}
        EnumFlags(Enum bit) : mask_(static_cast<typename std::underlying_type<Enum>::type>(bit)) {}
        EnumFlags(typename std::underlying_type<Enum>::type flags) : mask_{ flags } {}
        EnumFlags(const EnumFlags<Enum>& rhs) : mask_(rhs.mask_) {}
        EnumFlags<Enum>& operator=(const EnumFlags<Enum>& rhs) { mask_ = rhs.mask_; return *this; }

        EnumFlags<Enum>& operator|=(const EnumFlags<Enum>& rhs) { mask_ |= rhs.mask_; return *this; }
        EnumFlags<Enum>& operator&=(const EnumFlags<Enum>& rhs) { mask_ &= rhs.mask_; return *this; }
        EnumFlags<Enum>& operator^=(const EnumFlags<Enum>& rhs) { mask_ ^= rhs.mask_; return *this; }
        EnumFlags<Enum> operator|(const EnumFlags<Enum>& rhs) const { EnumFlags<Enum> result(*this); result |= rhs; return result; }
        EnumFlags<Enum> operator&(const EnumFlags<Enum>& rhs) const { EnumFlags<Enum> result(*this); result &= rhs; return result; }
        EnumFlags<Enum> operator^(const EnumFlags<Enum>& rhs) const { EnumFlags<Enum> result(*this); result ^= rhs; return result; }
        bool operator!() const { return !mask_; }
        EnumFlags<Enum> operator~() const { return ~mask_; }
        bool operator==(const EnumFlags<Enum>& rhs) const { return mask_ == rhs.mask_; }
        bool operator!=(const EnumFlags<Enum>& rhs) const { return mask_ != rhs.mask_; }
        explicit operator bool() const { return !!mask_; }
        explicit operator typename std::underlying_type<Enum>::type() const { return mask_; }

    private:
        /** Holds the bit mask. */
        typename std::underlying_type<Enum>::type mask_;
    };

    template<typename Enum>
    typename std::enable_if<EnableBitMaskOperators<Enum>::enable, EnumFlags<Enum>>::type operator|(Enum lhs, Enum rhs)
    {
        return EnumFlags<Enum>(lhs) | rhs;
    }

    template<typename Enum>
    typename std::enable_if<EnableBitMaskOperators<Enum>::enable, EnumFlags<Enum>>::type operator~(Enum rhs)
    {
        return ~(EnumFlags<Enum>(static_cast<typename std::underlying_type<Enum>::type>(rhs)));
    }
}
