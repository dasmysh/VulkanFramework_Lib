/**
* @file    stbfix.cpp
* @author  Sebastian Maisch <sebastian.maisch@googlemail.com>
* @date    03.04.2016
*
* @brief   Global location to add implementations of stb headers.
*/

#pragma warning(push)
#pragma warning(disable: 4244)
#define STB_IMAGE_IMPLEMENTATION
// ReSharper disable once CppUnusedIncludeDirective
#include <stb_image.h>
#pragma warning(pop)
