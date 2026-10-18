/// @file vulkan_logging.h
/// @brief Printing Vulkan information through the Logger.
///
/// The author adds these as member functions of Logger. We keep them in their
/// own file (one file per topic), so logger.h stays free of Vulkan and the
/// window/app code doesn't have to compile vulkan.hpp.
/// Every function does nothing when the logger is disabled.
#pragma once
#include <vulkan/vulkan.hpp>
#include <cstdint>
#include <vector>

/// Functions that print Vulkan objects and lists through the Logger.
namespace vklog {

	/// Print a version number packed by VK_MAKE_API_VERSION, e.g. "1.4.309".
	void report_version_number(std::uint32_t version);

	/// Print a list of names, one per line, each with a tab in front.
	void print_list(const std::vector<const char*>& names);

	/// Print the names of the extensions Vulkan reports. (Stage 3)
	void print_extensions(const std::vector<vk::ExtensionProperties>& extensions);
	/// Print the names of the layers Vulkan reports. (Stage 3)
	void print_layers(const std::vector<vk::LayerProperties>& layers);

	/// Print a GPU's name, type and supported Vulkan version. (Stage 4)
	void log(const vk::PhysicalDevice& device);

	/// Print each queue family: what it can do, how many queues. (Stage 5)
	void log(const std::vector<vk::QueueFamilyProperties>& queueFamilies);

	/// Print a surface's image counts, size, transform, alpha and usage. (Stage 7)
	void log(const vk::SurfaceCapabilitiesKHR& capabilities);
	/// Print the pixel formats a surface supports. (Stage 7)
	void log(const std::vector<vk::SurfaceFormatKHR>& formats);
	/// Print the present modes a surface supports. (Stage 7)
	void log(const std::vector<vk::PresentModeKHR>& modes);

}
