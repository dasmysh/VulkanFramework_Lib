/**
 * @file   Configuration.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.16
 *
 * @brief  Definition of the configuration class for windows systems.
 */

#pragma once

#include <string>
#include <boost/archive/xml_oarchive.hpp>
// ReSharper disable once CppUnusedIncludeDirective
#include <boost/serialization/vector.hpp>

namespace vku {

    namespace cfg {

        class WindowCfg
        {
        public:
            WindowCfg();
            WindowCfg(const WindowCfg&);
            WindowCfg(WindowCfg&&);
            WindowCfg& operator=(const WindowCfg&);
            WindowCfg& operator=(WindowCfg&&);
            ~WindowCfg();

            /** Holds the window title. */
            std::string windowTitle_;
            /** Holds whether the main window is full screen. */
            bool fullscreen_;
            /** Holds the bit depth of the back-buffer. */
            int backbufferBits_;
            /** Holds the windows left position. */
            int windowLeft_;
            /** Holds the windows top position. */
            int windowTop_;
            /** Holds the windows width. */
            int windowWidth_;
            /** Holds the windows height. */
            int windowHeight_;
            /** Holds whether the back buffer should use sRGB. */
            bool useSRGB_;

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
                ar & BOOST_SERIALIZATION_NVP(windowTitle_);
                ar & BOOST_SERIALIZATION_NVP(fullscreen_);
                ar & BOOST_SERIALIZATION_NVP(backbufferBits_);
                ar & BOOST_SERIALIZATION_NVP(windowLeft_);
                ar & BOOST_SERIALIZATION_NVP(windowTop_);
                ar & BOOST_SERIALIZATION_NVP(windowWidth_);
                ar & BOOST_SERIALIZATION_NVP(windowHeight_);
                ar & BOOST_SERIALIZATION_NVP(useSRGB_);
            }

            /**
            * Saving method for boost serialization.
            * @param ar the archive to serialize to.
            * @param version the archives version.
            */
            template<class Archive>
            void load(Archive & ar, const unsigned int)
            {
                ar & BOOST_SERIALIZATION_NVP(windowTitle_);
                ar & BOOST_SERIALIZATION_NVP(fullscreen_);
                ar & BOOST_SERIALIZATION_NVP(backbufferBits_);
                ar & BOOST_SERIALIZATION_NVP(windowLeft_);
                ar & BOOST_SERIALIZATION_NVP(windowTop_);
                ar & BOOST_SERIALIZATION_NVP(windowWidth_);
                ar & BOOST_SERIALIZATION_NVP(windowHeight_);
                ar & BOOST_SERIALIZATION_NVP(useSRGB_);
            }

            BOOST_SERIALIZATION_SPLIT_MEMBER()
        };


        class Configuration
        {
        public:
            Configuration();
            Configuration(const Configuration&);
            Configuration(Configuration&&);
            Configuration& operator=(const Configuration&);
            Configuration& operator=(Configuration&&);
            ~Configuration();

            /** Holds configurations for each window. */
            std::vector<WindowCfg> windows_;
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
                ar & BOOST_SERIALIZATION_NVP(pauseOnKillFocus_);
                ar & BOOST_SERIALIZATION_NVP(resourceBase_);
                ar & BOOST_SERIALIZATION_NVP(resourceDirs_);
                ar & BOOST_SERIALIZATION_NVP(evalDirectory_);
            }

            /**
             * Saving method for boost serialization.
             * @param ar the archive to serialize to.
             * @param version the archives version.
             */
            template<class Archive>
            void load(Archive & ar, const unsigned int)
            {
                ar & BOOST_SERIALIZATION_NVP(pauseOnKillFocus_);
                ar & BOOST_SERIALIZATION_NVP(resourceBase_);
                ar & BOOST_SERIALIZATION_NVP(resourceDirs_);
                ar & BOOST_SERIALIZATION_NVP(evalDirectory_);
            }

            BOOST_SERIALIZATION_SPLIT_MEMBER()
        };
    }
}

BOOST_CLASS_VERSION(vku::cfg::WindowCfg, 1)
BOOST_CLASS_VERSION(vku::cfg::Configuration, 1)
