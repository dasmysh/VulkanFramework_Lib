/**
* @file    Resource.cpp
* @author  Sebastian Maisch <sebastian.maisch@googlemail.com>
* @date    2014.03.29
*
* @brief   Implementations for the resource class.
*/

#include "Resource.h"
#include <boost/regex.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/filesystem.hpp>
#include "app/ApplicationBase.h"
#include "app/Configuration.h"

namespace vku {
    /**
     * Constructor.
     * @param resourceId the resource id to use
     */
    Resource::Resource(const std::string& resourceId, gfx::LogicalDevice* device) :
        device_{ device },
        id_{ GetNormalizedResourceId(resourceId) }
    {
        boost::split(subresourceIds_, id_, boost::is_any_of("|"));
        for (auto& sr : subresourceIds_) boost::trim(sr);

        if (subresourceIds_.size() == 1) {
            boost::split(parameters_, subresourceIds_[0], boost::is_any_of(","));
            for (auto& param : parameters_) {
                boost::trim(param);
                std::string name, value;
                if (parseNamedValue(param, name, value)) {
                    namedParameters_.insert(std::make_pair(name, value));
                } else if (parseNamedFlag(param, name)) {
                    namedParameters_.insert(std::make_pair(name, ""));
                }
            }
        }
    };

    /** Default copy constructor. */
    Resource::Resource(const Resource&) = default;

    /** Default assignment operator. */
    Resource& Resource::operator=(const Resource&) = default;

    /** Move constructor. */
    Resource::Resource(Resource&& orig) noexcept :
        device_{ orig.device_ },
        id_{ std::move(orig.id_) }
    {
    };

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

    std::string Resource::GetNamedParameterString(const std::string& name) const
    {
        auto it = namedParameters_.find(name);
        if (it != namedParameters_.end()) return it->second;
        return "";
    }

    bool Resource::CheckNamedParameterFlag(const std::string& name) const
    {
        return namedParameters_.find(name) != namedParameters_.end();
    }

    bool Resource::parseNamedValue(const std::string& str, std::string& name, std::string& value)
    {
        const boost::regex nameValueRegex("^-([\\w-]*)=(.*)$");
        boost::smatch paramMatch;
        if (boost::regex_match(str, paramMatch, nameValueRegex)) {
            name = paramMatch[1].str();
            value = paramMatch[2].str();
            return true;
        }
        return false;
    }

    bool Resource::parseNamedFlag(const std::string& str, std::string& name)
    {
        const boost::regex flagRegex("^-([\\w-]*)$");
        boost::smatch paramMatch;
        if (boost::regex_match(str, paramMatch, flagRegex)) {
            name = paramMatch[1].str();
            return true;
        }
        return false;
    }

    /**
         *  Returns the normalized resource id (no global parameters).
         *  @param resId the resource id.
         *  @return the normalized resource id.
         */
    std::string Resource::GetNormalizedResourceId(const std::string& resId)
    {
        std::vector<std::string> subresourcesAndGlobalParams;
        boost::split_regex(subresourcesAndGlobalParams, resId, boost::regex("\\|,"));
        for (auto& sr : subresourcesAndGlobalParams) boost::trim(sr);

        SubResourceList subresources;
        boost::split(subresources, subresourcesAndGlobalParams[0], boost::is_any_of("|"));
        for (auto& sr : subresources) {
            boost::trim(sr);
            if (subresourcesAndGlobalParams.size() > 1) sr += "," + subresourcesAndGlobalParams[1];
        }
        return boost::join(subresources, "|");
    }

    /**
     *  Returns the actual location of the resource by looking into all search paths.
     *  @param localFilename the file name local to any resource base directory.
     *  @return the path to the resource.
     */
    std::string Resource::FindResourceLocation(const std::string& localFilename) const
    {
        auto filename = ApplicationBase::instance().GetConfig().resourceBase_ + "/" + localFilename;
        if (boost::filesystem::exists(filename)) return filename;

        for (const auto& dir : ApplicationBase::instance().GetConfig().resourceDirs_) {
            filename = dir + "/" + localFilename;
            if (boost::filesystem::exists(filename)) return filename;
        }

        LOG(FATAL) << L"Cannot find local resource file \"" << localFilename.c_str() << L"\".";
        throw resource_loading_error() << ::boost::errinfo_file_name(localFilename) << resid_info(id_)
            << errdesc_info("Cannot find local resource file.");
    }
}
