Stage 2

# The Vulkan instance

Video: "Vulkan with C++, Stage 2: Instance Creation", https://www.youtube.com/watch?v=c3CggmiSGsA
Author's code: `updatedGetIntoGameDev\APIs\vulkan\02 - instance\finished`
WebGPU counterpart: `wgpu::Instance` in `MyLearnWebGPU\MyNotes\theAdapter.md` / `BasicProjectSetup.md`.

## What this stage does

Creates the first Vulkan object, the **instance**, and destroys it at exit. New output:
```
Making an instance...
Vulkan loader version 1.4.309 (variant 0)
Instance extensions to be requested:
	"VK_KHR_surface"
	"VK_KHR_win32_surface"
...
Goodbye see you!
Deleted instance
```

New files: `instance.{h,cpp}`, `deletion_queue.h`, `dispatch.cpp`, `../logging/vulkan_logging.{h,cpp}`.

## Vulkan ideas

### The instance
The instance is your program's connection to the Vulkan **loader** (`vulkan-1.dll`). It holds per-application state: which API version you're written for, which instance extensions and layers are on. Every other object is created from it, directly or indirectly. Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#initialization-instances

### Versions are packed into one number
`VK_MAKE_API_VERSION(variant, major, minor, patch)` packs four fields into 32 bits (3 + 7 + 10 + 12 bits). `vk::apiVersionMajor(v)` etc. unpack them. Your loader reports 1.4.309; the driver supports 1.4.312. Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#extendingvulkan-coreversions-versionnumbers

### `apiVersion` = the version YOU are written for
We ask for `VK_API_VERSION_1_3`, because Stage 10 uses Vulkan 1.3 features, and check that the loader is at least 1.3. The author asks for whatever the loader reports (bug 10). The spec: `apiVersion` "must be the highest version of Vulkan that the application is designed to use". https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#VkApplicationInfo

### Instance extensions
Everything beyond the core API is opt-in. Drawing to a window is an extension (a GPU can run with no screen at all): GLFW tells us which ones this operating system needs, `VK_KHR_surface` + `VK_KHR_win32_surface`. If Vulkan isn't usable, `glfwGetRequiredInstanceExtensions` returns NULL; the author doesn't check. https://www.glfw.org/docs/3.4/vulkan_guide.html#vulkan_ext

### Vulkan objects are handles you destroy yourself
`vk::Instance` is a thin wrapper around `VkInstance`, a handle. Nothing is destroyed automatically: every `create` needs a `destroy`, in reverse order. That's what `DeletionQueue` is for: each create pushes a lambda that undoes it, and the queue runs them newest first.

### The dispatcher (how Vulkan-Hpp finds functions)
Each Vulkan-Hpp call goes through a table of function pointers. We use one global *dynamic* table (`VULKAN_HPP_DEFAULT_DISPATCHER`) filled in three steps: `init(vkGetInstanceProcAddr)` here, `init(instance)` right after creating the instance, `init(device)` in Stage 5. Then core and extension functions are all called the same way. The author uses the static table plus a second dynamic one passed by hand to extension calls (Stage 3). Vulkan-Hpp README: https://github.com/KhronosGroup/Vulkan-Hpp#extensions--per-device-function-pointers

## What the video gets wrong (from the captions)

| Claim | Correction |
|---|---|
| "in Vulkan everything is just a 64-bit handle" | Only *non-dispatchable* handles are 64-bit numbers. *Dispatchable* ones (instance, physical device, device, queue, command buffer) are pointers to opaque objects. https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#fundamentals-objectmodel-overview |
| A `vk::Instance` captured in a lambda would "go out of scope" | `vk::Instance` copies exactly like `VkInstance` (one pointer-sized value). Capturing it by value is fine; what would be wrong is capturing `this` or a reference. |
| sType/pNext "aren't in the Vulkan-Hpp header" | Every `vk::` struct has them; they just aren't constructor parameters (sType is filled in for you). The author himself sets `pNext` in Stage 9. |
| Exceptions "incur extra overhead" | Opinion. With modern exception handling the success path is free; the cost of turning them off is checking every result by hand, which the code then mostly doesn't (bug 26). |
| (correct) The patch number of apiVersion is ignored | Yes, the spec says so. |

## C++ refresher

### 1. Lambdas and captures
```cpp
vk::Instance instance = ...;
deletionQueue.push([instance] {     // [instance]: COPY the handle into the lambda
    instance.destroy();
});
```
A lambda is an unnamed function object. The `[...]` list says which outside variables it may use: `[x]` copies, `[&x]` refers to the original (dangerous if the original dies first), `[this]` captures the object pointer. Our deleters copy handles, so they work whenever they run.

Backup example:
```cpp
int counter = 0;
auto byCopy = [counter] { return counter; };        // remembers 0
auto byRef  = [&counter] { return counter; };       // reads the live value
counter = 5;
byCopy();   // 0
byRef();    // 5
```
Source: https://en.cppreference.com/w/cpp/language/lambda

### 2. `std::function<void()>`
A type that can hold *any* callable with that signature (lambda, function pointer, functor). That's how one vector can store lambdas that captured different things. Cost: a small heap allocation per stored lambda, negligible here. Source: https://en.cppreference.com/w/cpp/utility/functional/function

### 3. `std::vector<const char*>` from a C array
```cpp
const char** names = glfwGetRequiredInstanceExtensions(&count);
std::vector<const char*> extensions(names, names + count);   // (first, one-past-last)
```
Pointer arithmetic: `names + count` points one past the last element. Many standard containers have this "iterator range" constructor. Source: https://en.cppreference.com/w/cpp/container/vector/vector (constructor 5)

### 4. `constexpr` and `inline constexpr`
`constexpr std::uint32_t requiredApiVersion = VK_API_VERSION_1_3;` is a compile-time constant. In a header (Stage 3's `enableValidation`) we write `inline constexpr` so every `.cpp` that includes it shares one definition. Source: https://en.cppreference.com/w/cpp/language/inline

### 5. Exceptions vs error codes
```cpp
vk::Instance instance = vk::createInstance(createInfo);   // throws vk::SystemError on failure
```
vs. the author's
```cpp
vk::ResultValue<vk::Instance> attempt = vk::createInstance(createInfo);   // with VULKAN_HPP_NO_EXCEPTIONS
if (attempt.result != vk::Result::eSuccess) { ... }
```
Both are valid styles. The rule that matters: if you choose error codes, check every one. Source: CG E.2/E.3, https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#S-errors

## What we changed from the author's code

| Author | Ours | Why |
|---|---|---|
| apiVersion = loader's version | `VK_API_VERSION_1_3`, loader checked | bug 10 |
| `VULKAN_HPP_NO_EXCEPTIONS` in some headers | exceptions on, set nothing per header | bugs 11, 26 |
| `return nullptr` on failure | throw | bug 26 |
| deletion queue emptied in `~Engine` | `DeletionQueue` class, emptied by its own destructor | bug 9 |
| `glfwGetRequiredInstanceExtensions` unchecked | NULL check | Stage 2 bug list |
| static dispatcher (+ a second one from Stage 3) | one dynamic dispatcher, `dispatch.cpp` | simpler, works for extensions |
| version printing in `Logger` | `vklog::` functions in `vulkan_logging.cpp` | one file per topic |
