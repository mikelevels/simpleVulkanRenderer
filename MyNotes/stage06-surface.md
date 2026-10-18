Stage 6

# The window surface

Video: "Vulkan with C++, Stage 6: Surfaces", https://www.youtube.com/watch?v=ecpmJSsXx8w
Author's code: `updatedGetIntoGameDev\APIs\vulkan\06 - surface\finished`
WebGPU counterpart: `glfwGetWGPUSurface` / `wgpu::Surface`, `MyLearnWebGPU\MyNotes\FirstColor.md`.

## What this stage does

Creates a **surface** for the GLFW window, and makes the queue-family search require "can present to this surface". New output at exit: `Deleted surface` (between the device and the messenger).

New files: `surface.{h,cpp}`.

## Vulkan ideas

### What a surface is
Vulkan has no idea what a window is. A `VkSurfaceKHR` wraps the operating system's window (a Win32 `HWND` here) so a swapchain can put images in it. It comes from the instance extensions of Stage 2 (`VK_KHR_surface`, `VK_KHR_win32_surface`). GLFW's `glfwCreateWindowSurface` calls the right platform function for us. Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#_wsi_surface · GLFW: https://www.glfw.org/docs/3.4/vulkan_guide.html#vulkan_surface

The surface belongs to the **instance** (created and destroyed with it), not to the device.

### Presentation support is per queue family
Not every queue family can show images on every surface. `getSurfaceSupportKHR(familyIndex, surface)` asks. Its answer is a `VkBool32`; the `VkResult` only says whether the question could be asked. The author checks the `VkResult` (bug 15), so every family looks like it can present.

### Why the surface is made before choosing the queue
The order in `make_instance_objects()` is: instance, messenger, surface. The surface is needed to pick a queue family that can both draw and present (Stage 5's search, extended here). On your GPU family 0 can do both; on some systems they differ, and you'd need two queues (the official tutorial covers that case: https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/01_Presentation/00_Window_surface.html).

### Thread safety
We create the surface on the render thread. GLFW allows it: `glfwCreateWindowSurface` "may be called from any thread". Source: https://www.glfw.org/docs/3.4/group__vulkan.html#ga1a24536bec3f80b08ead18e28e6ae965

## What the video gets wrong

| Claim | Correction |
|---|---|
| Checking the result "equal to success" tells us we can present | It tells us the query worked. The answer is the returned boolean (bug 15). |
| (correct) `GLFW_INCLUDE_VULKAN` / including Vulkan first is needed for `glfwCreateWindowSurface` | Yes: `glfw3.h` declares it only if Vulkan's header came first. Our `surface.cpp` includes `surface.h` (which includes `vulkan.hpp`) before `GLFW/glfw3.h`. |

## C++ refresher

### 1. Forward declarations
```cpp
struct GLFWwindow;   // in surface.h
vk::SurfaceKHR make_surface(vk::Instance instance, GLFWwindow* window, DeletionQueue& deletionQueue);
```
To use a *pointer* to a type, the compiler only needs to know the type exists, not what's inside. A forward declaration keeps `glfw3.h` out of `surface.h`, so files that include `surface.h` compile faster and don't get GLFW's macros. Source: https://en.cppreference.com/w/cpp/language/class#Forward_declaration

### 2. Include order can matter
Some headers change what they declare depending on what was included before them (`glfw3.h` checks for `VK_VERSION_1_0`). That's fragile, so we keep the dependency in one file (`surface.cpp`) with a comment saying why.

### 3. C handles and C++ wrappers
```cpp
VkSurfaceKHR rawSurface = VK_NULL_HANDLE;                         // C type
glfwCreateWindowSurface(instance, window, nullptr, &rawSurface);  // vk::Instance -> VkInstance automatically
vk::SurfaceKHR surface = rawSurface;                              // wrap it
```
Vulkan-Hpp's handle classes convert both ways, so C libraries like GLFW mix with C++ code easily.

### 4. Throwing a library's own exception type
```cpp
throw vk::SystemError(vk::make_error_code(static_cast<vk::Result>(result)), "glfwCreateWindowSurface");
```
We throw the same type Vulkan-Hpp throws, so the render thread's `catch (const std::exception&)` handles it like any other Vulkan failure, and the message names the error (e.g. `ErrorInitializationFailed`). `vk::SystemError` derives from `std::system_error`, which derives from `std::exception`. Source: https://en.cppreference.com/w/cpp/error/system_error

## What we changed from the author's code

| Author | Ours | Why |
|---|---|---|
| `glfwCreateWindowSurface` result ignored, uninitialized handle | checked, initialized | bug 17 |
| `getSurfaceSupportKHR(...).result == eSuccess` | the returned boolean | bug 15 |
| surface code inline in the Engine constructor | `surface.cpp` | one file per topic |
