/**
 * @file   Configuration.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.16
 *
 * @brief  Definition of the configuration class for windows systems.
 */

#pragma once

#include <cereal/archives/xml.hpp>
// ReSharper disable once CppUnusedIncludeDirective
#include <cereal/types/vector.hpp>

namespace vkfw_core::cfg {

    enum class SwapOptions
    {
        /** Use double buffering without v-sync. */
        DOUBLE_BUFFERING,
        /** Use double buffering with v-sync. */
        DOUBLE_BUFFERING_VSYNC,
        /** Use triple buffering (with v-sync). */
        TRIPLE_BUFFERING
    };
}

namespace cereal {

    template<class Archive> inline
    std::string save_minimal(Archive const &, ::vkfw_core::cfg::SwapOptions const & so)
    {
        std::string strValue;
        if (so == vkfw_core::cfg::SwapOptions::DOUBLE_BUFFERING) { strValue = "DOUBLE_BUFFERING"; }
        if (so == vkfw_core::cfg::SwapOptions::DOUBLE_BUFFERING_VSYNC) { strValue = "DOUBLE_BUFFERING_VSYNC"; }
        if (so == vkfw_core::cfg::SwapOptions::TRIPLE_BUFFERING) { strValue = "TRIPLE_BUFFERING"; }
        return strValue;
    }

    template<class Archive> inline
    void load_minimal(Archive const &, ::vkfw_core::cfg::SwapOptions& so, std::string const & strValue)
    {
        if (strValue == "DOUBLE_BUFFERING") { so = vkfw_core::cfg::SwapOptions::DOUBLE_BUFFERING; }
        if (strValue == "DOUBLE_BUFFERING_VSYNC") { so = vkfw_core::cfg::SwapOptions::DOUBLE_BUFFERING_VSYNC; }
        if (strValue == "TRIPLE_BUFFERING") { so = vkfw_core::cfg::SwapOptions::TRIPLE_BUFFERING; }
    }
}


namespace vkfw_core::cfg {

    constexpr std::size_t DEFAULT_SCREEN_SIZE_X = 800;
    constexpr std::size_t DEFAULT_SCREEN_SIZE_Y = 600;

    struct QueueCfg final
    {
        QueueCfg();
        QueueCfg(const QueueCfg&);
        QueueCfg(QueueCfg&&) noexcept;
        QueueCfg& operator=(const QueueCfg&);
        QueueCfg& operator=(QueueCfg&&) noexcept;
        ~QueueCfg();

        /** Holds whether queue has graphics capabilities. */
        bool graphics_ = true;
        /** Holds whether queue has compute capabilities. */
        bool compute_ = false;
        /** Holds whether queue has transfer capabilities. */
        bool transfer_ = true;
        /** Holds whether queue has sparse binding capabilities. */
        bool sparseBinding_ = false;
        /** Holds the queues priorities. */
        std::vector<float> priorities_ = {1.0};

        /**
        * Saving method for cereal.
        * @param ar the archive to serialize to.
        * @param version the archives version.
        */
        template<class Archive>
        void save(Archive & ar, std::uint32_t const) const
        {
            ar(cereal::make_nvp("graphicsCaps", graphics_),
                cereal::make_nvp("computeCaps", compute_),
                cereal::make_nvp("transferCaps", transfer_),
                cereal::make_nvp("sparseBindingCaps", sparseBinding_),
                cereal::make_nvp("priorities", priorities_));
        }

        /**
        * Saving method for cereal.
        * @param ar the archive to serialize to.
        * @param version the archives version.
        */
        template<class Archive>
        void load(Archive & ar, std::uint32_t const)
        {
            ar(cereal::make_nvp("graphicsCaps", graphics_),
                cereal::make_nvp("computeCaps", compute_),
                cereal::make_nvp("transferCaps", transfer_),
                cereal::make_nvp("sparseBindingCaps", sparseBinding_),
                cereal::make_nvp("priorities", priorities_));
        }
    };

    class WindowCfg final
    {
    public:
        WindowCfg();
        WindowCfg(const WindowCfg&);
        WindowCfg(WindowCfg&&) noexcept;
        WindowCfg& operator=(const WindowCfg&);
        WindowCfg& operator=(WindowCfg&&) noexcept;
        ~WindowCfg();

        /** Holds the window title. */
        std::string windowTitle_ = "VKFW_Application";
        /** Holds whether the main window is full screen. */
        bool fullscreen_ = false;
        /** Holds the windows left position. */
        std::size_t windowLeft_ = 0;
        /** Holds the windows top position. */
        std::size_t windowTop_ = 0;
        /** Holds the windows width. */
        std::size_t windowWidth_ = DEFAULT_SCREEN_SIZE_X;
        /** Holds the windows height. */
        std::size_t windowHeight_ = DEFAULT_SCREEN_SIZE_Y;
        /** Holds the bit depth of the back-buffer. */
        std::size_t backbufferBits_ = 32;
        /** Holds the bit depth of the depth buffer. */
        std::size_t depthBufferBits_ = 32;
        /** Holds the bit depth of the stencil buffer. */
        std::size_t stencilBufferBits_ = 0;
        /** Holds whether the back buffer should use sRGB. */
        bool useSRGB_ = false;
        /** Holds the swap options. */
        SwapOptions swapOptions_ = SwapOptions::DOUBLE_BUFFERING_VSYNC;
        /** Holds the queues needed. */
        std::vector<QueueCfg> queues_ = {QueueCfg{}};

        /**
        * Saving method for boost serialization.
        * @param ar the archive to serialize to.
        * @param version the archives version.
        */
        template<class Archive>
        void save(Archive & ar, std::uint32_t const) const
        {
            ar(cereal::make_nvp("title", windowTitle_),
                cereal::make_nvp("fullScreen", fullscreen_),
                cereal::make_nvp("positionLeft", windowLeft_),
                cereal::make_nvp("positionTop", windowTop_),
                cereal::make_nvp("width", windowWidth_),
                cereal::make_nvp("height", windowHeight_),
                cereal::make_nvp("backBufferBits", backbufferBits_),
                cereal::make_nvp("depthBufferBits", depthBufferBits_),
                cereal::make_nvp("stencilBufferBits", stencilBufferBits_),
                cereal::make_nvp("useSRGB", useSRGB_),
                cereal::make_nvp("swapOptions", swapOptions_),
                cereal::make_nvp("queues", queues_));
        }

        /**
        * Saving method for boost serialization.
        * @param ar the archive to serialize to.
        * @param version the archives version.
        */
        template<class Archive>
        void load(Archive & ar, std::uint32_t const)
        {
            ar(cereal::make_nvp("title", windowTitle_),
                cereal::make_nvp("fullScreen", fullscreen_),
                cereal::make_nvp("positionLeft", windowLeft_),
                cereal::make_nvp("positionTop", windowTop_),
                cereal::make_nvp("width", windowWidth_),
                cereal::make_nvp("height", windowHeight_),
                cereal::make_nvp("backBufferBits", backbufferBits_),
                cereal::make_nvp("depthBufferBits", depthBufferBits_),
                cereal::make_nvp("stencilBufferBits", stencilBufferBits_),
                cereal::make_nvp("useSRGB", useSRGB_),
                cereal::make_nvp("swapOptions", swapOptions_),
                cereal::make_nvp("queues", queues_));
        }
    };


    class Configuration final
    {
    public:
        Configuration();
        Configuration(const Configuration&);
        Configuration(Configuration&&) noexcept;
        Configuration& operator=(const Configuration&);
        Configuration& operator=(Configuration&&) noexcept;
        ~Configuration();

        /** Holds configurations for each window. */
        std::vector<WindowCfg> windows_ = {WindowCfg{}};
        /** Holds whether validation layers should be used (in release). */
        bool useValidationLayers_ = true;
        /** Holds whether the application should pause on focus loss of main (first) window. */
        bool pauseOnKillFocus_ = false;
        /** Holds the resource base directory. */
        std::string resourceBase_ = "resources";
        /** Holds the resource base directory. */
        std::vector<std::string> resourceDirs_;
        /** Holds the directory for evaluation results. */
        std::string evalDirectory_ = "evaluation";

        /**
         * Saving method for boost serialization.
         * @param ar the archive to serialize to.
         * @param version the archives version.
         */
        template<class Archive>
        void save(Archive & ar, std::uint32_t const) const
        {
            ar(cereal::make_nvp("windows", windows_),
                cereal::make_nvp("useValidationLayers", useValidationLayers_),
                cereal::make_nvp("pauseOnKillFocus", pauseOnKillFocus_),
                cereal::make_nvp("resourceBase", resourceBase_),
                cereal::make_nvp("resourceDirectories", resourceDirs_),
                cereal::make_nvp("evalDirectory", evalDirectory_));
        }

        /**
         * Saving method for boost serialization.
         * @param ar the archive to serialize to.
         * @param version the archives version.
         */
        template<class Archive>
        void load(Archive & ar, std::uint32_t const)
        {
            ar(cereal::make_nvp("windows", windows_),
                cereal::make_nvp("useValidationLayers", useValidationLayers_),
                cereal::make_nvp("pauseOnKillFocus", pauseOnKillFocus_),
                cereal::make_nvp("resourceBase", resourceBase_),
                cereal::make_nvp("resourceDirectories", resourceDirs_),
                cereal::make_nvp("evalDirectory", evalDirectory_));
        }
    };
}

// NOLINTNEXTLINE(cert-err58-cpp,misc-definitions-in-headers)
CEREAL_CLASS_VERSION(vkfw_core::cfg::QueueCfg, 1)
// NOLINTNEXTLINE(cert-err58-cpp,misc-definitions-in-headers)
CEREAL_CLASS_VERSION(vkfw_core::cfg::WindowCfg, 1)
// NOLINTNEXTLINE(cert-err58-cpp,misc-definitions-in-headers)
CEREAL_CLASS_VERSION(vkfw_core::cfg::Configuration, 1)
