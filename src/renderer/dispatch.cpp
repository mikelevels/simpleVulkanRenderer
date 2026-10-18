// dispatch.cpp: where Vulkan-Hpp's function-pointer table lives.
//
// Every Vulkan-Hpp call (instance.destroy(), device.createImageView(...))
// goes through a "dispatcher": a big struct of function pointers. We use the
// DYNAMIC dispatcher for everything (CMakeLists.txt sets
// VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1): its pointers are fetched at runtime
// with vkGetInstanceProcAddr / vkGetDeviceProcAddr, so extension functions
// (debug messenger, shader objects) work exactly like core ones.
//
// The table is one global object, VULKAN_HPP_DEFAULT_DISPATCHER, and exactly
// one .cpp file in the program must define its storage. This is that file.
// It is filled in three steps (see instance.cpp and device.cpp):
//   1. init(vkGetInstanceProcAddr)  -> functions that need no instance
//   2. init(instance)               -> instance functions
//   3. init(device)                 -> device functions, called directly
// Docs: https://github.com/KhronosGroup/Vulkan-Hpp#extensions--per-device-function-pointers
//
// The author instead uses the static dispatcher for core functions and a
// second vk::detail::DispatchLoaderDynamic object, passed by hand to every
// extension call ("dldi"). Same result, more parameters to carry around.
#include <vulkan/vulkan.hpp>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
