/**
 * @file   Configuration.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.16
 *
 * @brief  Implementation of the configuration class for windows systems.
 */

#include "app/Configuration.h"

namespace vku::cfg {

    QueueCfg::QueueCfg() :
        graphics_(true),
        compute_(false),
        transfer_(true),
        sparseBinding_(false),
        priorities_({ 1.0f })
    {}

    QueueCfg::QueueCfg(const QueueCfg& rhs) :
        graphics_(rhs.graphics_),
        compute_(rhs.compute_),
        transfer_(rhs.transfer_),
        sparseBinding_(rhs.sparseBinding_),
        priorities_(rhs.priorities_)
    {}

    QueueCfg::QueueCfg(QueueCfg&& rhs) noexcept :
    graphics_(std::move(rhs.graphics_)),
        compute_(std::move(rhs.compute_)),
        transfer_(std::move(rhs.transfer_)),
        sparseBinding_(std::move(rhs.sparseBinding_)),
        priorities_(std::move(rhs.priorities_))
    {}

    QueueCfg& QueueCfg::operator=(const QueueCfg& rhs)
    {
        if (this != &rhs) {
            auto tmp{ rhs };
            std::swap(*this, tmp);
        }
        return *this;
    }

    QueueCfg& QueueCfg::operator=(QueueCfg&& rhs) noexcept
    {
        if (this != &rhs) {
            this->~QueueCfg();
            graphics_ = std::move(rhs.graphics_);
            compute_ = std::move(rhs.compute_);
            transfer_ = std::move(rhs.transfer_);
            sparseBinding_ = std::move(rhs.sparseBinding_);
            priorities_ = std::move(rhs.priorities_);
        }
        return *this;
    }

    QueueCfg::~QueueCfg() = default;


    WindowCfg::WindowCfg() :
        windowTitle_{ "VKFW_Application" },
        fullscreen_(false),
        windowLeft_(0),
        windowTop_(0),
        windowWidth_(800),
        windowHeight_(600),
        backbufferBits_(32),
        depthBufferBits_(32),
        stencilBufferBits_(0),
        useSRGB_(false),
        swapOptions_(SwapOptions::DOUBLE_BUFFERING_VSYNC)
    {
        queues_.push_back(QueueCfg());
    }

    WindowCfg::WindowCfg(const WindowCfg& rhs) :
        windowTitle_(rhs.windowTitle_),
        fullscreen_(rhs.fullscreen_),
        windowLeft_(rhs.windowLeft_),
        windowTop_(rhs.windowTop_),
        windowWidth_(rhs.windowWidth_),
        windowHeight_(rhs.windowHeight_),
        backbufferBits_(rhs.backbufferBits_),
        depthBufferBits_(rhs.depthBufferBits_),
        stencilBufferBits_(rhs.stencilBufferBits_),
        useSRGB_(rhs.useSRGB_),
        swapOptions_(rhs.swapOptions_),
        queues_(rhs.queues_)
    {}

    WindowCfg::WindowCfg(WindowCfg&& rhs) noexcept :
    windowTitle_(std::move(rhs.windowTitle_)),
        fullscreen_(std::move(rhs.fullscreen_)),
        windowLeft_(std::move(rhs.windowLeft_)),
        windowTop_(std::move(rhs.windowTop_)),
        windowWidth_(std::move(rhs.windowWidth_)),
        windowHeight_(std::move(rhs.windowHeight_)),
        backbufferBits_(std::move(rhs.backbufferBits_)),
        depthBufferBits_(std::move(rhs.depthBufferBits_)),
        stencilBufferBits_(std::move(rhs.stencilBufferBits_)),
        useSRGB_(std::move(rhs.useSRGB_)),
        swapOptions_(std::move(rhs.swapOptions_)),
        queues_(std::move(rhs.queues_))
    {}

    WindowCfg& WindowCfg::operator=(const WindowCfg& rhs)
    {
        if (this != &rhs) {
            auto tmp{ rhs };
            std::swap(*this, tmp);
        }
        return *this;
    }

    WindowCfg& WindowCfg::operator=(WindowCfg&& rhs) noexcept
    {
        if (this != &rhs) {
            this->~WindowCfg();
            windowTitle_ = std::move(rhs.windowTitle_);
            fullscreen_ = std::move(rhs.fullscreen_);
            windowLeft_ = std::move(rhs.windowLeft_);
            windowTop_ = std::move(rhs.windowTop_);
            windowWidth_ = std::move(rhs.windowWidth_);
            windowHeight_ = std::move(rhs.windowHeight_);
            backbufferBits_ = std::move(rhs.backbufferBits_);
            depthBufferBits_ = std::move(rhs.depthBufferBits_);
            stencilBufferBits_ = std::move(rhs.stencilBufferBits_);
            useSRGB_ = std::move(rhs.useSRGB_);
            swapOptions_ = std::move(rhs.swapOptions_);
            queues_ = std::move(rhs.queues_);
        }
        return *this;
    }

    WindowCfg::~WindowCfg() = default;


    Configuration::Configuration() :
        useValidationLayers_(true),
        pauseOnKillFocus_(false),
        resourceBase_("resources"),
        evalDirectory_("evaluation")
    {
        windows_.emplace_back();
    }

    Configuration::Configuration(const Configuration& rhs) :
        windows_(rhs.windows_),
        useValidationLayers_(rhs.useValidationLayers_),
        pauseOnKillFocus_(rhs.pauseOnKillFocus_),
        resourceBase_(rhs.resourceBase_),
        resourceDirs_(rhs.resourceDirs_),
        evalDirectory_(rhs.evalDirectory_)
    {}

    Configuration::Configuration(Configuration&& rhs) noexcept :
    windows_(std::move(rhs.windows_)),
        useValidationLayers_(std::move(rhs.useValidationLayers_)),
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

    Configuration& Configuration::operator=(Configuration&& rhs) noexcept
    {
        if (this != &rhs) {
            this->~Configuration();
            windows_ = std::move(rhs.windows_);
            useValidationLayers_ = std::move(rhs.useValidationLayers_);
            pauseOnKillFocus_ = std::move(rhs.pauseOnKillFocus_);
            resourceBase_ = std::move(rhs.resourceBase_);
            resourceDirs_ = std::move(rhs.resourceDirs_);
            evalDirectory_ = std::move(rhs.evalDirectory_);
        }
        return *this;
    }

    Configuration::~Configuration() = default;
}
