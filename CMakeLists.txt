# CMakeLists.txt: the recipe CMake reads to generate a build (Visual Studio
# solution or Ninja files) for this project.
#
# We follow GetIntoGameDev's "Vulkan 2024" series (code in the repo's
# APIs/vulkan folder). The author builds on macOS with CMake in the first video
# and ships Visual Studio .sln files with a copied dependencies/ folder for
# Windows. We use CMake on Windows, like MyLearnWebGPU, because:
#   - one file describes the build for every IDE and for the command line;
#   - GLFW is built from source, so there is no prebuilt .lib that has to match
#     your compiler version;
#   - the Vulkan SDK is found through the VULKAN_SDK environment variable that
#     the LunarG installer sets, instead of a copied header folder.
# Docs: https://cmake.org/cmake/help/latest/module/FindVulkan.html

# 3.24 is the first CMake with COMPILE_WARNING_AS_ERROR (used below).
cmake_minimum_required(VERSION 3.24...3.31)

project(
	SimpleVulkanRenderer
	VERSION 0.1.0
	LANGUAGES CXX C   # C is needed because GLFW itself is written in C
)

# --- GLFW: the window library, built from the copy in glfw/ (version 3.4) ----
# We only want the library, not GLFW's own examples, tests and docs.
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

# --- Vulkan: headers + the loader library (vulkan-1.lib) from the LunarG SDK --
# find_package looks in $ENV{VULKAN_SDK} and creates the target Vulkan::Vulkan.
# (Not used by the code yet in Stage 1; Stage 2 makes the first Vulkan call.)
find_package(Vulkan REQUIRED)

# --- Threads: std::thread needs the platform's thread library -----------------
# On Windows this adds nothing; on Linux it adds -pthread. Asking for it keeps
# the project portable. Docs: https://cmake.org/cmake/help/latest/module/FindThreads.html
find_package(Threads REQUIRED)

# --- The program ---------------------------------------------------------------
# Same folders as the author's src/: backend, controller, logging, renderer.
add_executable(App
	src/main.cpp
	src/backend/glfw_backend.h
	src/backend/glfw_backend.cpp
	src/controller/app.h
	src/controller/app.cpp
	src/logging/logger.h
	src/logging/logger.cpp
	src/renderer/renderer.h
	src/renderer/renderer.cpp
	src/renderer/render_thread.h
	src/renderer/render_thread.cpp
	# Stage 2: the instance
	src/logging/vulkan_logging.h
	src/logging/vulkan_logging.cpp
	src/renderer/deletion_queue.h
	src/renderer/dispatch.cpp
	src/renderer/instance.h
	src/renderer/instance.cpp
	# Stage 3: validation layers
	src/logging/debug_messenger.h
	src/logging/debug_messenger.cpp
	# Stage 4: physical device
	src/renderer/device.h
	src/renderer/device.cpp
)

target_link_libraries(App PRIVATE glfw Vulkan::Vulkan Threads::Threads)

# Vulkan-Hpp settings, given to EVERY file at once. Setting them with #define
# inside one header (as the author does with VULKAN_HPP_NO_EXCEPTIONS) only
# works if that header is always included first; two files that see different
# settings get different versions of the same functions (bugsExplained.md, 11).
target_compile_definitions(App PRIVATE
	# Fetch every Vulkan function pointer at runtime (see dispatch.cpp).
	VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
)

# C++20: the author moves to C++20 (stdcpp20) in later stages; we start there.
target_compile_features(App PRIVATE cxx_std_20)
set_target_properties(App PROPERTIES
	CXX_EXTENSIONS OFF            # plain ISO C++, no compiler-specific dialect
	COMPILE_WARNING_AS_ERROR ON   # a warning stops the build: we read them all
)

# Turn on (almost) all warnings. /W4 is MSVC, -Wall -Wextra is GCC/Clang.
if (MSVC)
	target_compile_options(App PRIVATE /W4)
else()
	target_compile_options(App PRIVATE -Wall -Wextra -pedantic)
endif()

# Make App the project Visual Studio starts when you press F5.
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT App)
