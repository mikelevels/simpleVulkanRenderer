// vulkan_logging.h: printing Vulkan information through the Logger.
//
// The author adds these as member functions of Logger. We keep them in their
// own file (one file per topic), so logger.h stays free of Vulkan and the
// window/app code doesn't have to compile vulkan.hpp.
// Every function does nothing when the logger is disabled.
#pragma once
#include <vulkan/vulkan.hpp>
#include <cstdint>
#include <vector>

namespace vklog {

	// Print a version number packed by VK_MAKE_API_VERSION, e.g. "1.4.309".
	void report_version_number(std::uint32_t version);

	// Print a list of names, one per line, each with a tab in front.
	void print_list(const std::vector<const char*>& names);

}
