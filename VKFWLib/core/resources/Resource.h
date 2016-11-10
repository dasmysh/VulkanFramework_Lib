/**
 * @file   Resource.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2013.12.31
 *
 * @brief  Contains Resource, a base class for all managed resources.
 */

#pragma once

#include "main.h"
#include <boost/lexical_cast.hpp>

namespace vku {
    namespace gfx {
        class LogicalDevice;
    }

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
        const std::string& GetFilename() const { return parameters_[0]; }

    protected:
        /** A list of sub-resources. */
        using SubResourceList = std::vector<std::string>;
        /** A list of parameters. */
        using ParameterList = std::vector<std::string>;
        /**  A map of flag and value parameters. */
        using ParameterMap = std::unordered_map<std::string, std::string>;

        std::string FindResourceLocation(const std::string& localFilename) const;
        const ParameterList& GetParameters() const { return parameters_; };
        const SubResourceList& GetSubresourceIds() const { return subresourceIds_; };
        std::string GetParameter(unsigned int index) const { return parameters_[index]; };
        std::string GetNamedParameterString(const std::string& name) const;
        template<typename T> T GetNamedParameterValue(const std::string& name, const T& def) const { 
            auto resultString = GetNamedParameterString(name);
            T result = def;
            if (resultString.size() != 0) result = boost::lexical_cast<T>(resultString);
            return result;
        };
        bool CheckNamedParameterFlag(const std::string& name) const;

        static bool parseNamedValue(const std::string& str, std::string& name, std::string& value);
        static bool parseNamedFlag(const std::string& str, std::string& name);

        /** Holds the device object for dependencies. */
        const gfx::LogicalDevice* device_;

    private:
        static std::string GetNormalizedResourceId(const std::string& resId);

        /** Holds the resources id. */
        std::string id_;
        /** Holds the sub-resources ids. */
        SubResourceList subresourceIds_;
        /** Holds the parameters. */
        ParameterList parameters_;
        /** Holds the named parameters with (optional) values. */
        ParameterMap namedParameters_;
    };
}
