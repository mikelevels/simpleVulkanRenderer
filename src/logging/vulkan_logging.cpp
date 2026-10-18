// vulkan_logging.cpp: how Vulkan information is printed.
#include "vulkan_logging.h"
#include "logger.h"
#include <sstream>

namespace vklog {

	void report_version_number(std::uint32_t version) {
		Logger& logger = Logger::get_logger();
		if (!logger.is_enabled()) {
			return;   // don't even build the string
		}
		// A Vulkan version is ONE 32-bit number with four fields packed in:
		//   bits 29-31 variant (0 = Vulkan), 22-28 major, 12-21 minor, 0-11 patch
		// The apiVersion* helpers shift and mask them back out.
		// Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#extendingvulkan-coreversions-versionnumbers
		std::ostringstream line;
		line << "Vulkan loader version " << vk::apiVersionMajor(version)
			<< "." << vk::apiVersionMinor(version)
			<< "." << vk::apiVersionPatch(version)
			<< " (variant " << vk::apiVersionVariant(version) << ")";
		logger.print(line.str());
	}

	void print_list(const std::vector<const char*>& names) {
		Logger& logger = Logger::get_logger();
		for (const char* name : names) {
			std::ostringstream line;
			line << "\t\"" << name << "\"";
			logger.print(line.str());
		}
	}

	void print_extensions(const std::vector<vk::ExtensionProperties>& extensions) {
		Logger& logger = Logger::get_logger();
		// `const auto&`: look at each element where it is, no copy. The
		// author writes `for (vk::ExtensionProperties extension : extensions)`
		// which copies a 260-byte struct per element.
		for (const auto& extension : extensions) {
			std::ostringstream line;
			// extensionName is a fixed-size char array wrapper; .data() is
			// the C string inside it.
			line << "\t\"" << extension.extensionName.data() << "\"";
			logger.print(line.str());
		}
	}

	void print_layers(const std::vector<vk::LayerProperties>& layers) {
		Logger& logger = Logger::get_logger();
		for (const auto& layer : layers) {
			std::ostringstream line;
			line << "\t\"" << layer.layerName.data() << "\"";
			logger.print(line.str());
		}
	}

}
