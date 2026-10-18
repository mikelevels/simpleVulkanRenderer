Stage 4

# Choosing a GPU (the physical device)

Video: "Vulkan with C++, Stage 4: Physical Device Selection", https://www.youtube.com/watch?v=YJfR-kMqzew
Author's code: `updatedGetIntoGameDev\APIs\vulkan\04 - physical device\finished`
WebGPU counterpart: requesting the adapter, `MyLearnWebGPU\MyNotes\theAdapter.md`. A WebGPU adapter is the same idea as a Vulkan physical device.

## What this stage does

Lists the GPUs Vulkan can see and picks one that supports everything we need. New output:
```
Choosing physical device...
Device: NVIDIA TITAN Xp (DiscreteGpu, Vulkan 1.4)
```

New files: `device.{h,cpp}`.

## Vulkan ideas

### Physical devices are found, not created
`instance.enumeratePhysicalDevices()` returns one handle per GPU (and per software implementation, if installed). You never destroy them. Each can be asked:
- **properties** (`getProperties`): name, type (discrete / integrated / CPU), supported Vulkan version, limits;
- **extensions** (`enumerateDeviceExtensionProperties`);
- **features** (`getFeatures2`, Stage 9);
- **queue families** (Stage 5).

Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#devsandqueues-physical-device-enumeration

### What makes a GPU "suitable" for us
| Requirement | Stage | Why |
|---|---|---|
| Vulkan 1.3 or newer | 4 | we request 1.3 (Stage 2); 1.3 features in Stage 10 |
| `VK_KHR_swapchain` | 4 | showing images in a window (Stage 7) |
| `VK_EXT_shader_object` + feature `shaderObject` | 9 | shaders without pipelines |
| feature `dynamicRendering` | 10 | drawing without render passes |

The list lives in one function, `required_device_extensions()`, so Stage 5 enables exactly what Stage 4 checked.

### Preferring a discrete GPU
Laptops often have two GPUs: an integrated one (in the CPU, slow, low power) and a discrete one (its own card and memory). The author takes the first suitable GPU in the list; we take the first, but let a discrete GPU replace it. Your PC has one GPU, so the result is the same.

## What the video gets wrong

| Claim | Correction |
|---|---|
| In OpenGL you "had to write an ICD" to select a GPU | An ICD ("installable client driver") is the GPU vendor's driver; applications never write one. OpenGL simply has no way to pick a GPU: the OS or driver decides. https://github.com/KhronosGroup/Vulkan-Loader/blob/main/docs/LoaderInterfaceArchitecture.md |
| Portability enumeration added for everyone | `VK_KHR_portability_enumeration` is only needed on macOS (MoltenVK). We're on Windows, so it's left out. The author removes it again in Stage 8. |

## C++ refresher

### 1. Range-based `for` and `continue`
```cpp
for (vk::PhysicalDevice device : instance.enumeratePhysicalDevices()) {
    if (!is_suitable(device)) {
        continue;   // skip to the next device
    }
    ...
}
```
The loop works directly on the vector returned by the function call; the temporary vector lives until the loop ends. Source: https://en.cppreference.com/w/cpp/language/range-for

### 2. `const auto&` in loops
`for (const auto& extension : available)` looks at each element where it is. `for (vk::ExtensionProperties extension : available)` (the author's style) copies each 260-byte struct. Rule of thumb: `const auto&` for anything bigger than a couple of pointers. Source: CG ES.71/F.16, https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rf-in

### 3. Short-circuit `&&` and `||`
```cpp
return supports_extensions(device, required_device_extensions())
    && supports_features(device);
```
If the first test is false, the second never runs. Here that's required, not just faster: asking for the features of an extension the GPU doesn't have is not allowed. Source: https://en.cppreference.com/w/cpp/language/operator_logical

### 4. A handle that can be "empty"
```cpp
vk::PhysicalDevice chosen = nullptr;
if (!chosen) { ... }   // vk:: handles convert to bool: false when null
```
Source: Vulkan-Hpp README, handles, https://github.com/KhronosGroup/Vulkan-Hpp#handles

## What we changed from the author's code

| Author | Ours | Why |
|---|---|---|
| first device with the extensions | also Vulkan 1.3 (and features from Stage 9), discrete preferred | bug 14 |
| returns `nullptr` if none, caller continues | throws | bug 26 |
| hand-written switch over device types | `vk::to_string(properties.deviceType)` | shorter, can't fall out of date |
| portability enumeration (macOS) | left out | Windows only |
| `malloc`'d array of extension names | `std::vector<const char*>` | bug 25 |
