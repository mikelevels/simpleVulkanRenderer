# simpleVulkanRenderer: the plan

## What this project is

A C++ Vulkan renderer learned by following GetIntoGameDev's **"Vulkan 2024"** series ("Vulkan with C++, Stage N"), one stage per commit, on Windows, in the same style as MyLearnWebGPU:
- `main.cpp` holds only `main()`; one file per topic; no function longer than 25 lines;
- heavy teaching comments; every refactor of the author's code is explained as best practice;
- every stage is built and run (Debug and Release, as the author recommends for threaded code) before it is proposed, and you say "commit" yourself;
- every bug in the author's code is fixed and written up in `bugsExplained.md` (bad vs good code, symptom, why);
- every topic has sources and a backup example, written for someone relearning C++ (your request of 2026-10-10).

Later, this code becomes the second renderer backend (`engine/vulkan/`) next to the WebGPU one (`engine/webgpu/`), in the renderer-centric engine you chose on 2026-10-08.

## Where we are (2026-10-10)

Stages 1 to 10 are done: the triangle is on screen (`images/stage10-triangle.png`). You asked to fast-track to here, then review everything before going on. Each stage was built and run in Debug and Release, and saved separately, so each becomes its own commit when you say "commit". Next: Stage 11 (runtime shader compilation), then 12, then the stop before 13.

## Decisions (2026-10-10)

1. **Series: Vulkan 2024**, not the 2022 one I started with. Your words: "Follow the 2024 playlist."
2. **GPU check before every stage:** if a stage needs something your GPU lacks, I tell you before writing any of it. Result of the check of all 30 stages: see "GPU features" below. Nothing is missing.
3. **Windows, not macOS.** The first video ("Making a Vulkan cmake project in macOS") is skipped; our CMake setup replaces it. The author's macOS-only code (`VK_KHR_portability_subset`, `VK_EXT_metal_surface`) is noted and left out.
4. **Stop before Stage 13** (Multithreaded Rendering): you review multithreading first, then we go through it line by line.

## Sources

| What | Where |
|---|---|
| Author's code (read-only reference) | `C:\Users\gonza\Dev\Practice\updatedGetIntoGameDev\APIs\vulkan\NN - name` (clone of https://github.com/amengede/getIntoGameDev, commit 18808e1 of 2025-12-30) |
| Videos | https://www.youtube.com/playlist?list=PLn3eTxaOtL2Nr89hYzKPib7tvce-ZO4yB |
| Captions | read with the `youtube-transcript-api` Python package (installed 2026-10-10 with your OK) |
| The Vulkan spec (the final word on every rule) | https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html |
| Official Vulkan tutorial (second opinion on each topic) | https://docs.vulkan.org/tutorial/latest/ |
| Vulkan-Hpp (the C++ wrapper the author uses) | https://github.com/KhronosGroup/Vulkan-Hpp |
| C++ language reference | https://en.cppreference.com |
| C++ Core Guidelines (best-practice rules, cited as "CG") | https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines |
| Your WebGPU notes (same ideas, other API) | `MyLearnWebGPU\MyNotes\*.md`, `updatedMyLearnWebGPU\MyNotes\{milestone1,bugsExplained}.md` |

## The stages

| Stage | Video | Folder | Status |
|---|---|---|---|
| - | Making a Vulkan cmake project in macOS | - | skipped (macOS); our CMakeLists.txt instead |
| 1 | App Structure | 01 - app structure | done, awaiting commit |
| 2 | Instance Creation | 02 - instance | done, awaiting commit |
| 3 | Validation Layers | 03 - validation layers | done, awaiting commit |
| 4 | Physical Device Selection | 04 - physical device | done, awaiting commit |
| 5 | Logical Device Creation | 05 - logical device | done, awaiting commit |
| 6 | Surfaces | 06 - surface | done, awaiting commit |
| 7 | Swapchain | 07 - swapchain | done, awaiting commit |
| 8 | Image Views | 08 - image views | done, awaiting commit |
| 9 | Shader Objects | 09 - shaders | done, awaiting commit |
| 10 | Dynamic Rendering | 10 - command buffers | done, awaiting commit |
| 11 | Runtime Shader Compilation | 11 - spirv compilation | |
| 12 | Synchronization | 12 - synchronization | |
| **13** | **Multithreaded Rendering** | 13 - multithreaded rendering (two versions: conservative, aggressive) | **STOP before this one: your review of multithreading first, then line by line together** |
| 14 | Frames in Flight | 14 - frames in flight | |
| 15 | Swapchain Recreation | 15 - swapchain recreation | |
| 16 | Vertex Buffers | 16-Vertex Buffers | |
| 17 | Staging Buffers | 17-Staging Buffers | |
| - | What's on the horizon for my Vulkan tutorials? | - | talk only |
| 18 | Descriptor Set Layouts | 18-Descriptor-Set-Layout | |
| 19 | Pipeline Layouts | 19-Pipeline-Layout | |
| 20 | Compute Shader Creation | 20-Compute-Shader | |
| 21 | Descriptor Pools & Sets | 21-Descriptor-Pools-and-Sets | |
| 22 | Storage Images | 22-Storage-Images | |
| 23 | Clearing the screen with a compute shader | 23-Compute-Shader-Clear-Screen | |
| 24 | Storage Buffers | 24-storage-buffers | |
| 25 | Triangle Edges | 25-triangle-edges | |
| 26 | Barycentric Coordinates | 26-Barycentric-Coordinates | |
| 27 | Fragment Shader Emulation | 27-Fragment-Shader-Emulation | |
| 28 | Depth Buffering | 28-Depth-Buffer | |
| 29 | Atomic Image Operations | 29-Atomic-Images | |
| - | Vulkan Compute Shader Rasterization: OBJ models | 30 - obj models | |
| - | Vulkan Memory Alignment of Storage Buffers | - | |

From Stage 20 on the series draws triangles with compute shaders (a software rasterizer running on the GPU) instead of the fixed graphics pipeline.

**Note on the old "Multithreaded Rendering" chapter.** In the 2022 series, chapter 13 was called "Multithreaded Rendering", but it contains no threads: it's "frames in flight" (the CPU prepares one frame while the GPU draws another). The author has since renamed that folder `13 - Frames in Flight` (identical files), and says at the start of the 2024 Stage 13 video that the old one "wasn't really multi-threaded". The 2024 Stage 13 is real multithreading (`std::thread`, `std::mutex`, `std::atomic`, the Taskflow library).

## GPU features (checked 2026-10-10 against `vulkaninfo`)

| Needed by | Feature or extension | TITAN Xp |
|---|---|---|
| Stage 2+ | a Vulkan instance | loader 1.4.309: yes |
| Stage 12+ | Vulkan 1.3 core features (`PhysicalDeviceVulkan13Features`) | driver supports Vulkan 1.4.312: yes |
| Stage 7+ | `VK_KHR_swapchain` | yes |
| Stage 9+ | `VK_EXT_shader_object`, feature `shaderObject` | yes, native in the driver (not the emulation layer) |
| Stage 10+ | `VK_KHR_dynamic_rendering`, feature `dynamicRendering` | yes |
| Stage 10+ | `VK_KHR_synchronization2`, feature `synchronization2` | yes |
| Stage 11+ | shaderc (compiles GLSL at runtime) | in the Vulkan SDK: `shaderc_combined.lib` |
| Stage 13 | Taskflow (C++ task library, header-only) | to download at Stage 13 (a copy is already in `getIntoGameDev\ExternalDependencies\taskflow`) |
| Stage 28+ | `shaderInt64`, `shaderBufferInt64Atomics` | yes, yes |
| Stage 29+ | `VK_EXT_shader_image_atomic_int64`, `shaderImageInt64Atomics` | yes, yes |

Stage 7's code also prints the names of extensions like fragment shading rate and video decode; it only lists them, it doesn't use them.

## Toolchain (checked 2026-10-09)

| Tool | Version | Notes |
|---|---|---|
| Vulkan SDK | 1.3.283.0 (`C:\VulkanSDK\1.3.283.0`, `VULKAN_SDK` set) | 1.3.275.0 also installed, unused |
| GPU / driver | NVIDIA TITAN Xp, driver 582.28, Vulkan 1.4.312 | |
| Vulkan loader | 1.4.309 | |
| Validation layer | VK_LAYER_KHRONOS_validation 1.3.283 | from the SDK |
| CMake / compiler | VS 2022's CMake 3.31.6, MSVC 17.14, C++20 | |
| GLFW | 3.4, source copied from MyLearnWebGPU/glfw | |
