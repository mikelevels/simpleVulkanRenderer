Stage 7

# The swapchain

Video: "Vulkan with C++, Stage 7: Swapchain", https://www.youtube.com/watch?v=38_GukoUL1k
Author's code: `updatedGetIntoGameDev\APIs\vulkan\07 - swapchain\finished`
WebGPU counterpart: `surface.configure(...)` in `MyLearnWebGPU\MyNotes\FirstColor.md`. WebGPU hides the swapchain inside the surface; Vulkan makes you build it.

## What this stage does

Asks the surface what it supports, chooses a format, size, image count and present mode, creates the **swapchain**, and makes one `Frame` per swapchain image. New output (your PC):
```
Surface capabilities:
	image count: min 2, max 8 (0 = no limit)
	current extent: 800x600
	current transform: Identity
	supported alpha: { Opaque }
	supported usage: { TransferSrc | TransferDst | Sampled | Storage | ColorAttachment | InputAttachment }
	supported format: B8G8R8A8Unorm, color space: SrgbNonlinear
	supported format: B8G8R8A8Srgb, color space: SrgbNonlinear
	... 3 more ...
	supported present mode: Fifo
	supported present mode: FifoRelaxed
	supported present mode: Mailbox
	supported present mode: Immediate
	supported present mode: invalid ( 3ba04c28 )
Swapchain has 3 images
...
Deleted swapchain
```
The last mode is real: 1000361000, `VK_PRESENT_MODE_FIFO_LATEST_READY_EXT`, a mode newer than our SDK's headers (1.3.283). `vk::to_string` prints it as "invalid ( 3ba04c28 )"; since the 2026-10-15 review `vulkan_logging.cpp` names it itself. Your driver (1.4.312) supports it.

New files: `swapchain.{h,cpp}`, `frame.{h,cpp}`; `glfw_backend` now measures the window in pixels on the main thread.

## Vulkan ideas

### What the swapchain is
A small set of images owned by the presentation engine. Each frame: borrow one (acquire, Stage 10), draw into it, give it back to be shown (present). While the screen shows one image, you draw into another, so you never see a half-drawn frame. Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#_wsi_swapchain · Tutorial: https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/01_Presentation/01_Swap_chain.html

### The four choices
| Choice | Ours (= author's) | Notes |
|---|---|---|
| **Format** | `B8G8R8A8Unorm` + `SrgbNonlinear` | see "UNORM vs SRGB" below |
| **Extent** (size) | `currentExtent` = 800x600 | on Windows the surface dictates it; 0xFFFFFFFF would mean "you choose" |
| **Image count** | min + 1 = 3 (max 8) | one spare, so we rarely wait for a free image; bug 18 for `max == 0` |
| **Present mode** | Mailbox, else FIFO | below |

### Present modes
| Mode | Behavior |
|---|---|
| FIFO | queue of images shown one per monitor refresh (v-sync); if the queue is full, the app WAITS. The only mode every GPU must support. |
| FIFO relaxed | like FIFO, but a late image is shown immediately (may tear) |
| Mailbox | never waits: a newer finished image replaces the waiting one. Lowest latency, no tearing, but the GPU renders frames nobody sees. |
| Immediate | shown at once, may tear |

Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#VkPresentModeKHR

### UNORM vs SRGB (a choice to revisit)
Monitors expect sRGB-encoded values (brightness isn't linear). With an `...Srgb` swapchain format the GPU encodes your shader's linear colors for you; with `...Unorm` the values go to the screen as-is, so mid-tones look darker than intended. The author uses UNORM; we keep it so colors match his videos, and it doesn't matter for a flat-color triangle. For lighting later, SRGB is the usual choice. Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#VkColorSpaceKHR · Explanation: https://learnopengl.com/Advanced-Lighting/Gamma-Correction

### Pixels vs screen coordinates, and GLFW's main-thread rule
The swapchain needs the window's size in **pixels** (`glfwGetFramebufferSize`), which on a high-DPI screen differs from its size in screen coordinates (`glfwGetWindowSize`, used by the author). Both functions must be called on the main thread, but the Engine runs on the render thread (bug 19). So `GlfwWindow` measures once on the main thread, and `RenderThread` copies the numbers into the new thread.

## What the video gets wrong

| Claim | Correction |
|---|---|
| currentExtent = max uint32 means it failed | It means "the size is up to you" (e.g. Wayland). https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#VkSurfaceCapabilitiesKHR |
| FIFO "fails" when nothing is ready | It blocks (waits); nothing fails. |
| A default-constructed Swapchain gets "sensible default values like zero" | `uint32_t imageCount;` with no initializer is left indeterminate (garbage) in a default-initialized object; only members with `{}` or `= 0` get values. Ours: `std::uint32_t imageCount{ 0 };`. https://en.cppreference.com/w/cpp/language/default_initialization |
| Images are "blitted over" to the window | How images reach the screen is up to the driver (page flip, composition, copy). The API doesn't say. |

## C++ refresher

### 1. Default member initializers prevent garbage
```cpp
struct Bad  { std::uint32_t count; };       // Bad b;  -> b.count is garbage
struct Good { std::uint32_t count{ 0 }; };  // Good g; -> g.count is 0
```
Reading an indeterminate value is undefined behavior. Source: CG C.48, https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rc-in-class-initializer

### 2. `static` member functions
`choose_present_mode` etc. are `static`: they don't use any member, so they don't get a `this`. That documents "pure function of its arguments" and lets you call them without an object. Source: https://en.cppreference.com/w/cpp/language/static#Static_member_functions

### 3. `std::clamp` (C++17)
```cpp
std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width)
```
replaces the author's `std::min(max, std::max(min, value))`. Source: https://en.cppreference.com/w/cpp/algorithm/clamp

### 4. `.at(i)` vs `[i]`
`formats.at(0)` throws `std::out_of_range` if the vector is empty; `formats[0]` on an empty vector is undefined behavior. Use `.at()` when the size isn't guaranteed. Source: https://en.cppreference.com/w/cpp/container/vector/at

### 5. `emplace_back` and `reserve`
```cpp
frames.reserve(images.size());            // allocate once
frames.emplace_back(image, ...);          // construct in place: Frame(image, ...)
```
`push_back(Frame(...))` builds a temporary and moves it in; `emplace_back` builds the element directly inside the vector. `reserve` avoids reallocating (and moving all Frames) as it grows. Sources: https://en.cppreference.com/w/cpp/container/vector/emplace_back · https://en.cppreference.com/w/cpp/container/vector/reserve

### 6. Capture handles, not `this`
The author's swapchain deleter captures `this` and reads `chain` when it runs; that works only as long as the Swapchain object never moves. Ours copies the handle: `[logicalDevice, handle] { logicalDevice.destroySwapchainKHR(handle); }`.

## What we changed from the author's code

| Author | Ours | Why |
|---|---|---|
| `std::min(max, min + 1)` | min + 1, capped only if max > 0 | bug 18 |
| `glfwGetWindowSize` on the render thread | `glfwGetFramebufferSize` on the main thread, passed in | bug 19 |
| `formats[0]` fallback | `formats.at(0)` | empty list -> exception, not UB |
| `uint32_t imageCount;` | `{ 0 }` | no garbage |
| deleter captures `this` | captures the handle | survives moves |
| hand-written flag printing | `vk::to_string` on flags | shorter |

## Update (2026-10-15 review)

The present mode is now FIFO by default (`preferredPresentMode` in `swapchain.h`); the measurements behind that are in `stage10b-framePacing.md`.
