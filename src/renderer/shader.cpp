/// @file shader.cpp
/// @brief Creating shader objects from SPIR-V files.
#include "shader.h"
#include "../backend/file.h"
#include "../logging/logger.h"

namespace {

/// Describe one shader stage. `code` must stay alive until createShadersEXT
/// has returned, because the info only points at it.
vk::ShaderCreateInfoEXT describe_stage(vk::ShaderStageFlagBits stage,
	vk::ShaderStageFlags nextStage, const std::vector<char>& code) {
	vk::ShaderCreateInfoEXT info;
	// eLinkStage: the two stages are compiled together, so the driver can
	// optimize across them (like a pipeline). Linked shaders must be created
	// in the SAME createShadersEXT call.
	info.flags = vk::ShaderCreateFlagBitsEXT::eLinkStage;
	info.stage = stage;
	info.nextStage = nextStage;   // which stage may come after this one
	// eSpirv: portable SPIR-V from glslc. (eBinary means a driver-specific
	// binary saved from an earlier run with getShaderBinaryDataEXT, a cache.)
	info.codeType = vk::ShaderCodeTypeEXT::eSpirv;
	info.codeSize = code.size();
	info.pCode = code.data();     // SPIR-V must be 4-byte aligned: new[] is
	info.pName = "main";          // the entry point function in the GLSL
	return info;
}

}

std::vector<vk::ShaderEXT> make_shader_objects(vk::Device logicalDevice,
	const char* vertexFilename, const char* fragmentFilename,
	DeletionQueue& deletionQueue) {
	const std::vector<char> vertexCode = read_file(vertexFilename);
	const std::vector<char> fragmentCode = read_file(fragmentFilename);
	const std::vector<vk::ShaderCreateInfoEXT> infos = {
		describe_stage(vk::ShaderStageFlagBits::eVertex, vk::ShaderStageFlagBits::eFragment, vertexCode),
		describe_stage(vk::ShaderStageFlagBits::eFragment, {}, fragmentCode),
	};
	// In our SDK's Vulkan-Hpp, createShadersEXT returns a ResultValue (it can
	// also succeed with eIncompatibleShaderBinaryEXT); errors still throw.
	std::vector<vk::ShaderEXT> shaders = logicalDevice.createShadersEXT(infos).value;
	for (const vk::ShaderEXT shader : shaders) {
		deletionQueue.push([logicalDevice, shader] {
			logicalDevice.destroyShaderEXT(shader);
		});
	}
	Logger::get_logger().print("Successfully made shaders");
	return shaders;
}
