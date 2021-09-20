/**
 * @file   main.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.16
 *
 * @brief  Includes common headers and defines common templates.
 */

#pragma once

#pragma warning(disable: 4514 4711 4996)

#pragma warning(push, 3)
#include <array>
// ReSharper disable CppUnusedIncludeDirective
#include <cassert>
#include <cmath>
#include <cstdint>
#include <deque>
#include <exception>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <span>
#pragma warning(pop)

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <vulkan/vulkan.hpp>

#include <spdlog/spdlog.h>

#include "constants.h"
#include "app/Configuration.h"
#include "core/resources/Resource.h"
#include "core/resources/ResourceManager.h"

namespace vkfw_core {
    class ApplicationBase;
}
// ReSharper restore CppUnusedIncludeDirective
