/// @file shader.h
/// @brief Shader objects (VK_EXT_shader_object). (Stage 9)
///
/// Classic Vulkan bakes the shaders AND almost all drawing state (blending,
/// culling, viewport...) into one big immutable "graphics pipeline" object.
/// The shader object extension (2023) lets you create shaders on their own,
/// bind them one stage at a time, and set all the other state with commands
/// while recording (Stage 10). It's closer to how OpenGL and WebGPU feel, and
/// it's what this series uses instead of pipelines.
/// Spec:  https://registry.khronos.org/vulkan/specs/latest/man/html/VK_EXT_shader_object.html
/// Intro: https://www.khronos.org/blog/you-can-use-vulkan-without-pipelines-today
#pragma once
#include "deletion_queue.h"
#include <vulkan/vulkan.hpp>
#include <vector>

/// Create a LINKED vertex + fragment shader pair from two SPIR-V files.
///
/// Pushes their destruction onto `deletionQueue`.
/// @return { vertex shader, fragment shader }
/// @throws std::runtime_error if a file can't be read
std::vector<vk::ShaderEXT> make_shader_objects(vk::Device logicalDevice,
	const char* vertexFilename, const char* fragmentFilename,
	DeletionQueue& deletionQueue);
