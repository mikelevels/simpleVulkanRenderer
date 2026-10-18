// dynamic_state.h: the drawing state that shader objects need us to set.
//
// With a graphics pipeline, the blend mode, cull mode, topology... are baked
// into the pipeline object. With shader objects (Stage 9) NONE of that state
// exists until we record a command that sets it, and drawing with any of it
// unset is an error. The author calls his version of this function
// "annoying_boilerplate_that_dynamic_rendering_was_meant_to_spare_us" (it's
// shader objects, not dynamic rendering, that require it; see stage10 notes).
// Spec, the list of required state: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#shaders-objects-state
#pragma once
#include <vulkan/vulkan.hpp>

// Record the state for drawing opaque, filled, uncull'd triangles covering
// an image of size `frameSize`, with no depth or stencil test.
void set_default_dynamic_state(vk::CommandBuffer commandBuffer, vk::Extent2D frameSize);
