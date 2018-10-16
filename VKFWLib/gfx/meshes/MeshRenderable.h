/**
 * @file   MeshInfo.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2014.01.13
 *
 * @brief  Contains the definition of the MeshInfo class.
 */

#pragma once

#include "main.h"
#include <typeindex>
#include "SubMesh.h"
#include "SceneMeshNode.h"
#include "gfx/Material.h"
#include <core/serialization_helper.h>
#include <cereal/cereal.hpp>

struct aiNode;

namespace vku::gfx {

    class DeviceBuffer;

}

