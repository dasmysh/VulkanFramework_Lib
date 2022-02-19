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
        bool m_graphics = true;
        /** Holds whether queue has compute capabilities. */
        bool m_compute = false;
        /** Holds whether queue has transfer capabilities. */
        bool m_transfer = true;
        /** Holds whether queue has sparse binding capabilities. */
        bool m_sparseBinding = false;
        /** Holds the queues priorities. */
        std::vector<float> m_priorities = {1.0};

        /**
        * Saving method for cereal.
        * @param ar the archive to serialize to.
        * @param version the archives version.
        */
        template<class Archive>
        void save(Archive & ar, std::uint32_t const) const
        {
            ar(cereal::make_nvp("graphicsCaps", m_graphics),
                cereal::make_nvp("computeCaps", m_compute),
                cereal::make_nvp("transferCaps", m_transfer),
                cereal::make_nvp("sparseBindingCaps", m_sparseBinding),
                cereal::make_nvp("priorities", m_priorities));
        }

        /**
        * Saving method for cereal.
        * @param ar the archive to serialize to.
        * @param version the archives version.
        */
        template<class Archive>
        void load(Archive & ar, std::uint32_t const)
        {
            ar(cereal::make_nvp("graphicsCaps", m_graphics),
                cereal::make_nvp("computeCaps", m_compute),
                cereal::make_nvp("transferCaps", m_transfer),
                cereal::make_nvp("sparseBindingCaps", m_sparseBinding),
                cereal::make_nvp("priorities", m_priorities));
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
        std::string m_windowTitle = "VKFW_Application";
        /** Holds whether the main window is full screen. */
        bool m_fullscreen = false;
        /** Holds the windows left position. */
        std::size_t m_windowLeft = 0;
        /** Holds the windows top position. */
        std::size_t m_windowTop = 0;
        /** Holds the windows width. */
        std::size_t m_windowWidth = DEFAULT_SCREEN_SIZE_X;
        /** Holds the windows height. */
        std::size_t m_windowHeight = DEFAULT_SCREEN_SIZE_Y;
        /** Holds the bit depth of the back-buffer. */
        std::size_t m_backbufferBits = 32;
        /** Holds the bit depth of the depth buffer. */
        std::size_t m_depthBufferBits = 32;
        /** Holds the bit depth of the stencil buffer. */
        std::size_t m_stencilBufferBits = 0;
        /** Holds whether the back buffer should use sRGB. */
        bool m_useSRGB = false;
        /** Holds the swap options. */
        SwapOptions m_swapOptions = SwapOptions::DOUBLE_BUFFERING_VSYNC;
        /** Indicates ray tracing support is needed for this window. */
        bool m_useRayTracing = false;
        /** Holds the queues needed. */
        std::vector<QueueCfg> m_queues = {QueueCfg{}};

        /**
        * Saving method for boost serialization.
        * @param ar the archive to serialize to.
        * @param version the archives version.
        */
        template<class Archive>
        void save(Archive & ar, std::uint32_t const) const
        {
            ar(cereal::make_nvp("title", m_windowTitle),
                cereal::make_nvp("fullScreen", m_fullscreen),
                cereal::make_nvp("positionLeft", m_windowLeft),
                cereal::make_nvp("positionTop", m_windowTop),
                cereal::make_nvp("width", m_windowWidth),
                cereal::make_nvp("height", m_windowHeight),
                cereal::make_nvp("backBufferBits", m_backbufferBits),
                cereal::make_nvp("depthBufferBits", m_depthBufferBits),
                cereal::make_nvp("stencilBufferBits", m_stencilBufferBits),
                cereal::make_nvp("useSRGB", m_useSRGB),
                cereal::make_nvp("swapOptions", m_swapOptions),
                cereal::make_nvp("useRayTracing", m_useRayTracing),
                cereal::make_nvp("queues", m_queues));
        }

        /**
        * Saving method for boost serialization.
        * @param ar the archive to serialize to.
        * @param version the archives version.
        */
        template<class Archive>
        void load(Archive & ar, std::uint32_t const version)
        {
            ar(cereal::make_nvp("title", m_windowTitle), cereal::make_nvp("fullScreen", m_fullscreen),
               cereal::make_nvp("positionLeft", m_windowLeft), cereal::make_nvp("positionTop", m_windowTop),
               cereal::make_nvp("width", m_windowWidth), cereal::make_nvp("height", m_windowHeight),
               cereal::make_nvp("backBufferBits", m_backbufferBits),
               cereal::make_nvp("depthBufferBits", m_depthBufferBits),
               cereal::make_nvp("stencilBufferBits", m_stencilBufferBits), cereal::make_nvp("useSRGB", m_useSRGB),
               cereal::make_nvp("swapOptions", m_swapOptions));
            if (version >= 2) ar(cereal::make_nvp("useRayTracing", m_useRayTracing));
            ar(cereal::make_nvp("queues", m_queues));
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
        std::vector<WindowCfg> m_windows = {WindowCfg{}};
        /** Holds whether validation layers should be used (in release). */
        bool m_useValidationLayers = true;
        /** Holds whether the application should pause on focus loss of main (first) window. */
        bool m_pauseOnKillFocus = false;
        /** Holds the resource base directory. */
        std::string m_resourceBase = "resources";
        /** Holds the resource base directory. */
        std::vector<std::string> m_resourceDirs;
        /** Holds the directory for evaluation results. */
        std::string m_evalDirectory = "evaluation";

        /**
         * Saving method for boost serialization.
         * @param ar the archive to serialize to.
         * @param version the archives version.
         */
        template<class Archive>
        void save(Archive & ar, std::uint32_t const) const
        {
            ar(cereal::make_nvp("windows", m_windows),
                cereal::make_nvp("useValidationLayers", m_useValidationLayers),
                cereal::make_nvp("pauseOnKillFocus", m_pauseOnKillFocus),
                cereal::make_nvp("resourceBase", m_resourceBase),
                cereal::make_nvp("resourceDirectories", m_resourceDirs),
                cereal::make_nvp("evalDirectory", m_evalDirectory));
        }

        /**
         * Saving method for boost serialization.
         * @param ar the archive to serialize to.
         * @param version the archives version.
         */
        template<class Archive>
        void load(Archive & ar, std::uint32_t const)
        {
            ar(cereal::make_nvp("windows", m_windows),
                cereal::make_nvp("useValidationLayers", m_useValidationLayers),
                cereal::make_nvp("pauseOnKillFocus", m_pauseOnKillFocus),
                cereal::make_nvp("resourceBase", m_resourceBase),
                cereal::make_nvp("resourceDirectories", m_resourceDirs),
                cereal::make_nvp("evalDirectory", m_evalDirectory));
        }
    };
}

// NOLINTNEXTLINE(cert-err58-cpp,misc-definitions-in-headers)
CEREAL_CLASS_VERSION(vkfw_core::cfg::QueueCfg, 1)
// NOLINTNEXTLINE(cert-err58-cpp,misc-definitions-in-headers)
CEREAL_CLASS_VERSION(vkfw_core::cfg::WindowCfg, 2)
// NOLINTNEXTLINE(cert-err58-cpp,misc-definitions-in-headers)
CEREAL_CLASS_VERSION(vkfw_core::cfg::Configuration, 1)
