/**
 * @file   Configuration.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.16
 *
 * @brief  Definition of the configuration class for windows systems.
 */

#pragma once

#include <boost/archive/xml_oarchive.hpp>
// ReSharper disable once CppUnusedIncludeDirective
#include <boost/serialization/vector.hpp>

namespace vku {
    namespace cfg {

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
}

BOOST_SERIALIZATION_SPLIT_FREE(vku::cfg::SwapOptions)

namespace boost {
    namespace serialization {

        template<class Archive>
        void save(Archive & ar, vku::cfg::SwapOptions& so, const unsigned int)
        {
            std::string strValue = "";
            if (so == vku::cfg::SwapOptions::DOUBLE_BUFFERING) strValue = "DOUBLE_BUFFERING";
            if (so == vku::cfg::SwapOptions::DOUBLE_BUFFERING_VSYNC) strValue = "DOUBLE_BUFFERING_VSYNC";
            if (so == vku::cfg::SwapOptions::TRIPLE_BUFFERING) strValue = "TRIPLE_BUFFERING";
            ar & strValue;
        }

        template<class Archive>
        void load(Archive & ar, vku::cfg::SwapOptions& so, const unsigned int)
        {
            std::string strValue = "";
            ar & strValue;
            if (strValue == "DOUBLE_BUFFERING") so = vku::cfg::SwapOptions::DOUBLE_BUFFERING;
            if (strValue == "DOUBLE_BUFFERING_VSYNC") so = vku::cfg::SwapOptions::DOUBLE_BUFFERING_VSYNC;
            if (strValue == "TRIPLE_BUFFERING") so = vku::cfg::SwapOptions::TRIPLE_BUFFERING;
        }
    }
}


namespace vku {
    namespace cfg {

        struct QueueCfg
        {
            QueueCfg();
            QueueCfg(const QueueCfg&);
            QueueCfg(QueueCfg&&) noexcept;
            QueueCfg& operator=(const QueueCfg&);
            QueueCfg& operator=(QueueCfg&&) noexcept;
            ~QueueCfg();

            /** Holds whether queue has graphics capabilities. */
            bool graphics_;
            /** Holds whether queue has compute capabilities. */
            bool compute_;
            /** Holds whether queue has transfer capabilities. */
            bool transfer_;
            /** Holds whether queue has sparse binding capabilities. */
            bool sparseBinding_;
            /** Holds the queues priorities. */
            std::vector<float> priorities_;

        private:
            /** Needed for serialization */
            friend class boost::serialization::access;

            /**
            * Saving method for boost serialization.
            * @param ar the archive to serialize to.
            * @param version the archives version.
            */
            template<class Archive>
            void save(Archive & ar, const unsigned int) const
            {
                ar & boost::serialization::make_nvp("graphicsCaps", graphics_);
                ar & boost::serialization::make_nvp("computeCaps", compute_);
                ar & boost::serialization::make_nvp("transferCaps", transfer_);
                ar & boost::serialization::make_nvp("sparseBindingCaps", sparseBinding_);
                ar & boost::serialization::make_nvp("priorities", priorities_);
            }

            /**
            * Saving method for boost serialization.
            * @param ar the archive to serialize to.
            * @param version the archives version.
            */
            template<class Archive>
            void load(Archive & ar, const unsigned int)
            {
                ar & boost::serialization::make_nvp("graphicsCaps", graphics_);
                ar & boost::serialization::make_nvp("computeCaps", compute_);
                ar & boost::serialization::make_nvp("transferCaps", transfer_);
                ar & boost::serialization::make_nvp("sparseBindingCaps", sparseBinding_);
                ar & boost::serialization::make_nvp("priorities", priorities_);
            }

            BOOST_SERIALIZATION_SPLIT_MEMBER()
        };

        class WindowCfg
        {
        public:
            WindowCfg();
            WindowCfg(const WindowCfg&);
            WindowCfg(WindowCfg&&) noexcept;
            WindowCfg& operator=(const WindowCfg&);
            WindowCfg& operator=(WindowCfg&&) noexcept;
            ~WindowCfg();

            /** Holds the window title. */
            std::string windowTitle_;
            /** Holds whether the main window is full screen. */
            bool fullscreen_;
            /** Holds the windows left position. */
            int windowLeft_;
            /** Holds the windows top position. */
            int windowTop_;
            /** Holds the windows width. */
            int windowWidth_;
            /** Holds the windows height. */
            int windowHeight_;
            /** Holds the bit depth of the back-buffer. */
            int backbufferBits_;
            /** Holds the bit depth of the depth buffer. */
            int depthBufferBits_;
            /** Holds the bit depth of the stencil buffer. */
            int stencilBufferBits_;
            /** Holds whether the back buffer should use sRGB. */
            bool useSRGB_;
            /** Holds the swap options. */
            SwapOptions swapOptions_;
            /** Holds the queues needed. */
            std::vector<QueueCfg> queues_;

        private:
            /** Needed for serialization */
            friend class boost::serialization::access;

            /**
            * Saving method for boost serialization.
            * @param ar the archive to serialize to.
            * @param version the archives version.
            */
            template<class Archive>
            void save(Archive & ar, const unsigned int) const
            {
                ar & boost::serialization::make_nvp("title", windowTitle_);
                ar & boost::serialization::make_nvp("fullScreen", fullscreen_);
                ar & boost::serialization::make_nvp("positionLeft", windowLeft_);
                ar & boost::serialization::make_nvp("positionTop", windowTop_);
                ar & boost::serialization::make_nvp("width", windowWidth_);
                ar & boost::serialization::make_nvp("height", windowHeight_);
                ar & boost::serialization::make_nvp("backBufferBits", backbufferBits_);
                ar & boost::serialization::make_nvp("depthBufferBits", depthBufferBits_);
                ar & boost::serialization::make_nvp("stencilBufferBits", stencilBufferBits_);
                ar & boost::serialization::make_nvp("useSRGB", useSRGB_);
                ar & boost::serialization::make_nvp("swapOptions", swapOptions_);
                ar & boost::serialization::make_nvp("queues", queues_);
            }

            /**
            * Saving method for boost serialization.
            * @param ar the archive to serialize to.
            * @param version the archives version.
            */
            template<class Archive>
            void load(Archive & ar, const unsigned int)
            {
                ar & boost::serialization::make_nvp("title", windowTitle_);
                ar & boost::serialization::make_nvp("fullScreen", fullscreen_);
                ar & boost::serialization::make_nvp("positionLeft", windowLeft_);
                ar & boost::serialization::make_nvp("positionTop", windowTop_);
                ar & boost::serialization::make_nvp("width", windowWidth_);
                ar & boost::serialization::make_nvp("height", windowHeight_);
                ar & boost::serialization::make_nvp("backBufferBits", backbufferBits_);
                ar & boost::serialization::make_nvp("depthBufferBits", depthBufferBits_);
                ar & boost::serialization::make_nvp("stencilBufferBits", stencilBufferBits_);
                ar & boost::serialization::make_nvp("useSRGB", useSRGB_);
                ar & boost::serialization::make_nvp("swapOptions", swapOptions_);
                ar & boost::serialization::make_nvp("queues", queues_);
            }

            BOOST_SERIALIZATION_SPLIT_MEMBER()
        };


        class Configuration
        {
        public:
            Configuration();
            Configuration(const Configuration&);
            Configuration(Configuration&&) noexcept;
            Configuration& operator=(const Configuration&);
            Configuration& operator=(Configuration&&) noexcept;
            ~Configuration();

            /** Holds configurations for each window. */
            std::vector<WindowCfg> windows_;
            /** Holds whether validation layers should be used (in release). */
            bool useValidationLayers_;
            /** Holds whether the application should pause on focus loss of main (first) window. */
            bool pauseOnKillFocus_;
            /** Holds the resource base directory. */
            std::string resourceBase_;
            /** Holds the resource base directory. */
            std::vector<std::string> resourceDirs_;
            /** Holds the directory for evaluation results. */
            std::string evalDirectory_;

        private:
            /** Needed for serialization */
            friend class boost::serialization::access;

            /**
             * Saving method for boost serialization.
             * @param ar the archive to serialize to.
             * @param version the archives version.
             */
            template<class Archive>
            void save(Archive & ar, const unsigned int) const
            {
                ar & boost::serialization::make_nvp("windows", windows_);
                ar & boost::serialization::make_nvp("useValidationLayers", useValidationLayers_);
                ar & boost::serialization::make_nvp("pauseOnKillFocus", pauseOnKillFocus_);
                ar & boost::serialization::make_nvp("resourceBase", resourceBase_);
                ar & boost::serialization::make_nvp("resourceDirectories", resourceDirs_);
                ar & boost::serialization::make_nvp("evalDirectory", evalDirectory_);
            }

            /**
             * Saving method for boost serialization.
             * @param ar the archive to serialize to.
             * @param version the archives version.
             */
            template<class Archive>
            void load(Archive & ar, const unsigned int)
            {
                ar & boost::serialization::make_nvp("windows", windows_);
                ar & boost::serialization::make_nvp("useValidationLayers", useValidationLayers_);
                ar & boost::serialization::make_nvp("pauseOnKillFocus", pauseOnKillFocus_);
                ar & boost::serialization::make_nvp("resourceBase", resourceBase_);
                ar & boost::serialization::make_nvp("resourceDirectories", resourceDirs_);
                ar & boost::serialization::make_nvp("evalDirectory", evalDirectory_);
            }

            BOOST_SERIALIZATION_SPLIT_MEMBER()
        };
    }
}

BOOST_CLASS_VERSION(vku::cfg::QueueCfg, 1)
BOOST_CLASS_VERSION(vku::cfg::WindowCfg, 1)
BOOST_CLASS_VERSION(vku::cfg::Configuration, 1)
