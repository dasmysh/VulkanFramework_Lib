/**
 * @file   Configuration.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.16
 *
 * @brief  Implementation of the configuration class for windows systems.
 */

#include "Configuration.h"

namespace vku {
    namespace cfg {

        WindowCfg::WindowCfg() :
            windowTitle_{"VKFW_Application"},
            fullscreen_(false),
            backbufferBits_(32),
            windowLeft_(0),
            windowTop_(0),
            windowWidth_(800),
            windowHeight_(600),
            useSRGB_(false)
        {}

        WindowCfg::WindowCfg(const WindowCfg& rhs) :
            fullscreen_(rhs.fullscreen_),
            backbufferBits_(rhs.backbufferBits_),
            windowLeft_(rhs.windowLeft_),
            windowTop_(rhs.windowTop_),
            windowWidth_(rhs.windowWidth_),
            windowHeight_(rhs.windowHeight_),
            useSRGB_(rhs.useSRGB_)
        {}

        WindowCfg::WindowCfg(WindowCfg&& rhs) :
            fullscreen_(std::move(rhs.fullscreen_)),
            backbufferBits_(std::move(rhs.backbufferBits_)),
            windowLeft_(std::move(rhs.windowLeft_)),
            windowTop_(std::move(rhs.windowTop_)),
            windowWidth_(std::move(rhs.windowWidth_)),
            windowHeight_(std::move(rhs.windowHeight_)),
            useSRGB_(std::move(rhs.useSRGB_))
        {}

        WindowCfg& WindowCfg::operator=(const WindowCfg& rhs)
        {
            if (this != &rhs) {
                auto tmp{ rhs };
                std::swap(*this, tmp);
            }
            return *this;
        }

        WindowCfg& WindowCfg::operator=(WindowCfg&& rhs)
        {
            if (this != &rhs) {
                this->~WindowCfg();
                fullscreen_ = std::move(rhs.fullscreen_);
                backbufferBits_ = std::move(rhs.backbufferBits_);
                windowLeft_ = std::move(rhs.windowLeft_);
                windowTop_ = std::move(rhs.windowTop_);
                windowWidth_ = std::move(rhs.windowWidth_);
                windowHeight_ = std::move(rhs.windowHeight_);
                useSRGB_ = std::move(rhs.useSRGB_);
            }
            return *this;
        }

        WindowCfg::~WindowCfg() = default;


        Configuration::Configuration() :
            pauseOnKillFocus_(false),
            resourceBase_("resources"),
            evalDirectory_("evaluation")
        {
            windows_.emplace_back();
        }

        Configuration::Configuration(const Configuration& rhs) :
            windows_(rhs.windows_),
            pauseOnKillFocus_(rhs.pauseOnKillFocus_),
            resourceBase_(rhs.resourceBase_),
            resourceDirs_(rhs.resourceDirs_),
            evalDirectory_(rhs.evalDirectory_)
        {}

        Configuration::Configuration(Configuration&& rhs) :
            windows_(std::move(rhs.windows_)),
            pauseOnKillFocus_(std::move(rhs.pauseOnKillFocus_)),
            resourceBase_(std::move(rhs.resourceBase_)),
            resourceDirs_(std::move(rhs.resourceDirs_)),
            evalDirectory_(std::move(rhs.evalDirectory_))
        {}

        Configuration& Configuration::operator=(const Configuration& rhs)
        {
            if (this != &rhs) {
                auto tmp{ rhs };
                std::swap(*this, tmp);
            }
            return *this;
        }

        Configuration& Configuration::operator=(Configuration&& rhs)
        {
            if (this != &rhs) {
                this->~Configuration();
                windows_ = std::move(rhs.windows_);
                pauseOnKillFocus_ = std::move(rhs.pauseOnKillFocus_);
                resourceBase_ = std::move(rhs.resourceBase_);
                resourceDirs_ = std::move(rhs.resourceDirs_);
                evalDirectory_ = std::move(rhs.evalDirectory_);
            }
            return *this;
        }

        Configuration::~Configuration() = default;
    }
}
