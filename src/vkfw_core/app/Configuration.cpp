/**
 * @file   Configuration.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.16
 *
 * @brief  Implementation of the configuration class for windows systems.
 */

#include "app/Configuration.h"

namespace vku::cfg {

    QueueCfg::QueueCfg() = default;
    QueueCfg::QueueCfg(const QueueCfg& rhs) = default;
    QueueCfg::QueueCfg(QueueCfg&& rhs) noexcept = default;
    QueueCfg& QueueCfg::operator=(const QueueCfg& rhs) = default;
    QueueCfg& QueueCfg::operator=(QueueCfg&& rhs) noexcept = default;
    QueueCfg::~QueueCfg() = default;


    WindowCfg::WindowCfg() = default;
    WindowCfg::WindowCfg(const WindowCfg& rhs) = default;
    WindowCfg::WindowCfg(WindowCfg&& rhs) noexcept = default;
    WindowCfg& WindowCfg::operator=(const WindowCfg& rhs) = default;
    WindowCfg& WindowCfg::operator=(WindowCfg&& rhs) noexcept = default;
    WindowCfg::~WindowCfg() = default;


    Configuration::Configuration() = default;
    Configuration::Configuration(const Configuration& rhs) = default;
    Configuration::Configuration(Configuration&& rhs) noexcept = default;
    Configuration& Configuration::operator=(const Configuration& rhs) = default;
    Configuration& Configuration::operator=(Configuration&& rhs) noexcept = default;
    Configuration::~Configuration() = default;
}
