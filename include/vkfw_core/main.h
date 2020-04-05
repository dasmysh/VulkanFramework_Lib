/**
 * @file   main.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.16
 *
 * @brief  Includes common headers and defines common templates.
 */

#pragma once

#pragma warning(disable: 4514 4711 4996)

#if defined ( _MSC_VER )
// NOLINTNEXTLINE
#define __func__ __FUNCTION__
#endif

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

// static const unsigned int VK_NUMRETURN  = 0x100;
// static const unsigned int VK_NUMDECIMAL = 0x101;
// static const unsigned int VK_NUMDELETE  = 0x102;
//
// static const unsigned int VK_MOD_SHIFT = 0x01;
// static const unsigned int VK_MOD_CTRL = 0x02;
// static const unsigned int VK_MOD_MENU = 0x04;
// static const unsigned int VK_MOD_ALTGR = 0x08;
//
// static const unsigned int MB_LEFT = 0x01;
// static const unsigned int MB_RGHT = 0x02;
// static const unsigned int MB_MIDD = 0x04;
// static const unsigned int MB_BTN4 = 0x08;
// static const unsigned int MB_BTN5 = 0x10;