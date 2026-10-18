Stage 9

# Shaders and shader objects

Video: "Vulkan with C++, Stage 9: Shader Objects", https://www.youtube.com/watch?v=vWzvUWBIduY
Author's code: `updatedGetIntoGameDev\APIs\vulkan\09 - shaders\finished`
WebGPU counterpart: WGSL shader modules and the render pipeline, `MyLearnWebGPU\MyNotes\HelloTriangle.md`.

## What this stage does

Adds the triangle's two shaders, compiles them to SPIR-V when you build, loads them, and creates two linked **shader objects**. New output: `Successfully made shaders`. Nothing is drawn yet (Stage 10).

New files: `shaders/shader.vert`, `shaders/shader.frag`, `src/backend/file.{h,cpp}`, `src/renderer/shader.{h,cpp}`; a shader-compiling section in `CMakeLists.txt`.

## Vulkan ideas

### GLSL -> SPIR-V
We write shaders in GLSL. Vulkan doesn't read GLSL: it reads **SPIR-V**, a binary intermediate language (a list of 32-bit words). `glslc` (from the SDK, made by Google's shaderc project) translates. CMake now runs it whenever a shader changes:
```
glslc shaders/shader.vert -o build/shaders/vertex.spv
glslc shaders/shader.frag -o build/shaders/fragment.spv
```
Stage 11 moves compilation into the program itself (runtime compilation with shaderc). SPIR-V spec: https://registry.khronos.org/SPIR-V/ · glslc: https://github.com/google/shaderc/tree/main/glslc

### The two shaders
- **Vertex shader**: runs once per vertex; outputs a position. No vertex buffer yet (Stage 16): the three corners are a constant array, picked with `gl_VertexIndex` (0, 1, 2).
- **Fragment shader**: runs once per covered pixel; outputs a color. The vertex colors are blended across the triangle automatically (interpolation).

Vulkan's screen coordinates: x from -1 (left) to +1 (right), **y from -1 (top) to +1 (bottom)**. OpenGL and WebGPU point y up, so the same numbers draw upside down there. Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#vertexpostproc-viewport

### Pipelines vs shader objects
| | Graphics pipeline (classic, Vulkan 1.0) | Shader objects (`VK_EXT_shader_object`, 2023) |
|---|---|---|
| What you create | one object with ALL shaders + ALL state (blend, cull, depth, topology, attachment formats...) | one object per shader stage |
| Changing state | a new pipeline per combination, or declared dynamic states | set any state with a command while recording |
| Startup cost | compile every combination you'll need | compile shaders only |
| Used by | official tutorial, most engines | this series |

Shader objects feel like OpenGL/WebGPU and need far less setup; in exchange, every piece of state must be set before each draw (Stage 10). `eLinkStage` asks the driver to compile vertex + fragment together so it can optimize across them, like a pipeline. Linked shaders must be created in the same call. Spec: https://registry.khronos.org/vulkan/specs/latest/man/html/VK_EXT_shader_object.html · Khronos blog: https://www.khronos.org/blog/you-can-use-vulkan-without-pipelines-today

### Extensions need FEATURES turned on too
Supporting `VK_EXT_shader_object` (the extension) doesn't switch it on. The device must also be created with the **feature** `shaderObject = VK_TRUE`, chained through `DeviceCreateInfo::pNext`. We first check with `getFeatures2` that the GPU supports it (the author enables it without checking, bug 14). Your TITAN Xp supports it natively (`vulkaninfo`: `shaderObject = true`); GPUs without it could use the SDK's `VK_LAYER_KHRONOS_shader_object` emulation layer. Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#features

## What the video gets wrong

| Claim | Correction |
|---|---|
| The "binary" code type is for making your own custom binary format | `eBinary` means a driver-specific binary previously saved with `vkGetShaderBinaryDataEXT` (a cache), valid only on the same device and driver. https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#shaders-objects-binary-code |
| Drawing to an image of a different size means recreating the pipeline | Viewport and scissor have been dynamic state since Vulkan 1.0; a size change doesn't need a new pipeline. |
| With the static dispatcher it compiles, then can't find the function at runtime | You'd get a LINK error: `vkCreateShadersEXT` is declared but not exported by `vulkan-1.lib`. (Our dynamic dispatcher avoids both.) |
| Descriptor set layouts are for "really large resources" | They describe any resource bindings (uniform buffers, textures, storage buffers), whatever their size. Push constants are the small, fast path. (Stage 18 covers it.) |

## C++ refresher

### 1. Reading a binary file
```cpp
std::ifstream file(filename, std::ios::ate | std::ios::binary);
std::streamsize size = file.tellg();     // position at the end = size
std::vector<char> buffer(size);
file.seekg(0);                           // back to the start
file.read(buffer.data(), size);
```
`ios::binary` matters on Windows: text mode would translate line endings and corrupt the data. The stream closes in its destructor. Source: https://en.cppreference.com/w/cpp/io/basic_ifstream

### 2. String literal concatenation
```cpp
SHADER_DIR "/vertex.spv"
```
`SHADER_DIR` is a macro from CMake that expands to a string literal (`"C:/Users/.../build/shaders"`). Two literals side by side are joined by the compiler into one: `"C:/.../shaders/vertex.spv"`. Source: https://en.cppreference.com/w/cpp/language/string_literal (concatenation)

### 3. `vk::StructureChain`
```cpp
vk::StructureChain<vk::DeviceCreateInfo, vk::PhysicalDeviceShaderObjectFeaturesEXT> chain(
    deviceInfo, vk::PhysicalDeviceShaderObjectFeaturesEXT(VK_TRUE));
physicalDevice.createDevice(chain.get<vk::DeviceCreateInfo>());
```
Holds several structs and links their `pNext` pointers in order. It also checks at compile time that each struct is allowed to extend the first one. The author links them by hand (`shaderFeatures.pNext = &dynamicFeatures; ...`), which is easy to get wrong. Source: https://github.com/KhronosGroup/Vulkan-Hpp#structure-pointer-chains

### 4. Keep the data alive while something points at it
```cpp
std::vector<char> vertexCode = read_file(vertexFilename);
info.pCode = vertexCode.data();   // a pointer INTO the vector
```
`info` doesn't own the code. If `vertexCode` were destroyed before `createShadersEXT` runs, `pCode` would dangle. That's why both vectors are locals of `make_shader_objects`, alive until the call returns.

## What we changed from the author's code

| Author | Ours | Why |
|---|---|---|
| `compile.bat` with his SDK path, run by hand | CMake runs `glslc` on change | bug 21 |
| `"shaders/vertex.spv"` relative path | absolute `SHADER_DIR` from CMake | bug 21 |
| `read_file` logs and continues | throws | bug 20 |
| `shaderObject` enabled without checking | checked with `getFeatures2` first | bug 14 |
| `pNext` linked by hand | `vk::StructureChain` | compile-time checked |
| shader z = 0.0 (Stage 9) then 0.1 (Stage 10) | 0.0 | inside the depth range either way; no depth test yet |
