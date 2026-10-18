Stage 3

# Validation layers and the debug messenger

Video: "Vulkan with C++, Stage 3: Validation Layers", https://www.youtube.com/watch?v=jKxy0V7Ukxk
Author's code: `updatedGetIntoGameDev\APIs\vulkan\03 - validation layers\finished`
WebGPU counterpart: WebGPU validates every call itself and reports through the device's uncaptured-error callback (`MyLearnWebGPU\MyNotes\theDevice.md`). Vulkan doesn't: you add a layer.

## What this stage does

In Debug builds: turns on the Khronos validation layer and routes its messages into our Logger. In Release builds: neither (faster, quieter). New output (Debug):
```
Layers to be requested:
	"VK_LAYER_KHRONOS_validation"
Instance can support these layers:
	... 16 layers on your PC ...
validation layer [Warning]: Layer name GalaxyOverlayVkLayer does not conform to naming standard (Policy #LLP_LAYER_3)
...
Deleted debug messenger
Deleted instance
```
The three warnings are real and harmless: GOG Galaxy installed a Vulkan layer whose name breaks the naming convention. They prove messages reach us even during `vkCreateInstance` (see "pNext" below).

New files: `../logging/debug_messenger.{h,cpp}`.

## Vulkan ideas

### Layers
A layer is a DLL the loader inserts between your calls and the driver. It sees every call. The Khronos validation layer (`VK_LAYER_KHRONOS_validation`, part of the SDK) checks each call against the spec's "Valid Usage" rules and reports violations with a `VUID-...` code you can search in the spec. Vulkan itself checks almost nothing, for speed: an invalid call is undefined behavior, not an error code. Guide: https://docs.vulkan.org/guide/latest/validation_overview.html

Other layers on your PC: NVIDIA's, Steam's overlay, OBS's capture hook, GOG Galaxy's, and SDK tools (api_dump prints every call; gfxreconstruct records them). Those come from installed programs, not from us.

### The debug messenger (`VK_EXT_debug_utils`)
An instance extension that delivers the layer's messages to a function of ours (`debug_callback`), filtered by severity (verbose, info, warning, error) and type (general, validation, performance). We take warnings and errors. Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#debugging-debug-utils

### Catching messages from instance creation itself: `pNext`
The messenger is created *after* the instance and destroyed *before* it, so on its own it can't report problems in `vkCreateInstance` / `vkDestroyInstance`. Chaining the same `DebugUtilsMessengerCreateInfoEXT` into `InstanceCreateInfo::pNext` covers those two calls. The author doesn't. `pNext` is Vulkan's general mechanism for "extra structs attached to this one", which we'll use again for device features in Stage 9.

### Checking support before asking
`enumerateInstanceExtensionProperties` / `enumerateInstanceLayerProperties` list what's available; asking for something missing makes `createInstance` fail with `VK_ERROR_EXTENSION_NOT_PRESENT` / `VK_ERROR_LAYER_NOT_PRESENT`. We check first, so the error names the missing item.

## What the video gets wrong

| Claim | Correction |
|---|---|
| A validation layer "can correct some very simple errors" | It only REPORTS. Your invalid call still goes to the driver. https://docs.vulkan.org/guide/latest/validation_overview.html |
| These are extensions "we're going to request upon device creation" | Instance extensions and layers, requested at `vkCreateInstance`. Device extensions come in Stage 4/5. |
| The severity list described as "everything" | The code asks for verbose + warning + error (no info). Verbose is the noisiest level; it floods the console with loader messages, which is why we leave it out. |
| The callback's return value means "whether to end program execution" (code comment) | It means "abort the Vulkan call that caused this message"; the spec says apps should always return VK_FALSE. https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#PFN_vkDebugUtilsMessengerCallbackEXT |

## C++ refresher

### 1. Function pointers and callbacks
`debug_callback` is passed to Vulkan as a *function pointer*; Vulkan calls it later. Its signature must match exactly what Vulkan expects:
```cpp
VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT*, void*);
```
`VKAPI_CALL` is the calling convention (on 32-bit Windows, `__stdcall`). The author casts his callback with `reinterpret_cast`, which would also accept a wrong signature (bug 12). Without the cast, the compiler checks it for us.

Backup example:
```cpp
int twice(int x) { return 2 * x; }
int apply(int (*f)(int), int v) { return f(v); }   // f: pointer to a function int(int)
apply(&twice, 21);   // 42
```
Source: https://en.cppreference.com/w/cpp/language/pointer#Pointers_to_functions

### 2. Unused parameters
Our callback leaves the 2nd and 4th parameters unnamed: `VkDebugUtilsMessageTypeFlagsEXT,`. An unnamed parameter can't be used, and the compiler doesn't warn about it (with `/W4` a named but unused one gives C4100). C++17's `[[maybe_unused]]` is the alternative.

### 3. Templates: one `contains` for two types
```cpp
template <typename Properties, typename NameOf>
bool contains(const std::vector<Properties>& available, const char* wanted, NameOf nameOf);
```
Works for `vk::ExtensionProperties` and `vk::LayerProperties`, whose name fields differ (`extensionName` vs `layerName`): the caller passes a small lambda that gets the name. The compiler writes one version per type it's used with. Source: https://en.cppreference.com/w/cpp/language/function_template

### 4. `#ifdef NDEBUG`
```cpp
#ifdef NDEBUG
inline constexpr bool enableValidation = false;
#else
inline constexpr bool enableValidation = true;
#endif
```
The preprocessor runs before the compiler and keeps only one branch. CMake's Release configuration defines `NDEBUG`; Debug doesn't. Source: https://en.cppreference.com/w/cpp/preprocessor/conditional

### 5. `std::strcmp` for C strings
`a == b` on two `const char*` compares the *pointers*, not the text. `std::strcmp(a, b) == 0` compares characters (header `<cstring>`; the author forgets the include). Source: https://en.cppreference.com/w/cpp/string/byte/strcmp

## What we changed from the author's code

| Author | Ours | Why |
|---|---|---|
| validation on when the logger is on | on in Debug, off in Release | bug 13 |
| `malloc` arrays of names | `std::vector<const char*>` | bug 25 |
| `reinterpret_cast` of the callback | exact C signature, no cast | bug 12 |
| second dispatcher `dldi`, passed by hand | the global dynamic dispatcher | Stage 2 |
| messenger only | also chained into `InstanceCreateInfo::pNext` | catches create/destroy messages |
| callback writes to `std::cerr` | through `Logger::print` (thread-safe) | the callback can run on any thread |
| verbose + warning + error | warning + error | verbose is noise |
