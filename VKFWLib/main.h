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
#define __func__ __FUNCTION__
#endif

#pragma warning(push, 3)
#include <array>
// ReSharper disable CppUnusedIncludeDirective
#include <cassert>
#include <cmath>
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

#pragma warning(disable : 4201)
#include <glm/glm.hpp>
#pragma warning(default : 4201)

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#pragma warning(push, 3)
// #include <Windows.h>
#pragma warning(pop)
#undef min
#undef max

#include <vulkan/vulkan.hpp>

#include <g3log/g3log.hpp>
#include <g3log/loglevels.hpp>

const LEVELS VK_GEN{ INFO.value + 2,{ "Vulkan" } };
const LEVELS VK_DEBUG{ DEBUG.value + 1,{ "Vulkan DEBUG" } };
const LEVELS VK_INFO{ INFO.value + 1,{ "Vulkan DEBUG" } };
const LEVELS VK_WARNING{ WARNING.value + 1,{ "Vulkan WARNING" } };
const LEVELS VK_PERF_WARNING{ WARNING.value + 2,{ "Vulkan PERFORMANCE WARNING" } };
const LEVELS VK_ERROR{ WARNING.value + 3,{ "Vulkan ERROR" } };

#ifdef VKUFW_EXPORT
#define VKUDllExport   __declspec( dllexport )
#else
#define VKUDllExport   __declspec( dllimport )
#endif

#include "constants.h"
#include "app/Configuration.h"
#include "core/resources/Resource.h"
#include "core/resources/ResourceManager.h"

namespace vku {
    class ApplicationBase;
}
// ReSharper restore CppUnusedIncludeDirective

static const unsigned int VK_NUMRETURN  = 0x100;
static const unsigned int VK_NUMDECIMAL = 0x101;
static const unsigned int VK_NUMDELETE  = 0x102;

static const unsigned int VK_MOD_SHIFT = 0x01;
static const unsigned int VK_MOD_CTRL = 0x02;
static const unsigned int VK_MOD_MENU = 0x04;
static const unsigned int VK_MOD_ALTGR = 0x08;

static const unsigned int MB_LEFT = 0x01;
static const unsigned int MB_RGHT = 0x02;
static const unsigned int MB_MIDD = 0x04;
static const unsigned int MB_BTN4 = 0x08;
static const unsigned int MB_BTN5 = 0x10;
