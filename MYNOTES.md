# simpleVulkanRenderer: my notes

Step-by-step log. One stage of GetIntoGameDev's "Vulkan 2024" series ("Vulkan with C++, Stage N") per commit, on Windows.

- `MyNotes/plan.md`: the stage list, the stop before Stage 13, the GPU check, sources, toolchain.
- `MyNotes/bugsExplained.md`: every bug in the author's code, bad vs good.
- `MyNotes/stageNN-*.md`: one file per stage, with the Vulkan ideas and a C++ refresher (sources + backup examples).

# Build (Visual Studio 2022's CMake, Vulkan SDK 1.3.283)

```
cmake -B build
cmake --build build --config Debug
cmake --build build --config Release
.\build\Debug\App.exe
```
Run threaded code in both Debug and Release (the author's advice, and ours).

# Log

| Stage | Notes | What it added |
|---|---|---|
| 1 App Structure | `MyNotes/stage01-appStructure.md` | CMake project, GLFW window, thread-safe logger, render thread (start/stop/join), app event loop |
| 2 Instance | `MyNotes/stage02-instance.md` | Vulkan instance, deletion queue, dynamic dispatcher |
| 3 Validation Layers | `MyNotes/stage03-validationLayers.md` | validation layer (Debug only), debug messenger |
| 4 Physical Device | `MyNotes/stage04-physicalDevice.md` | GPU selection (Vulkan 1.3, extensions, discrete preferred) |
| 5 Logical Device | `MyNotes/stage05-logicalDevice.md` | queue families, logical device, graphics queue |
| 6 Surface | `MyNotes/stage06-surface.md` | window surface, present support |
| 7 Swapchain | `MyNotes/stage07-swapchain.md` | swapchain, frames, window size in pixels |
| 8 Image Views | `MyNotes/stage08-imageViews.md` | one image view per swapchain image |
| 9 Shader Objects | `MyNotes/stage09-shaderObjects.md` | GLSL shaders compiled by CMake, linked shader objects |
| 10 Dynamic Rendering | `MyNotes/stage10-dynamicRendering.md` | command buffers, layouts, dynamic state, acquire/submit/present: **the triangle** |
| 10b Frame pacing (our lesson) | `MyNotes/stage10b-framePacing.md` | frames-per-second counter; FIFO by default: 60 fps at 0.03 core instead of 5,880 fps at 0.78 |
| 10 review | `MyNotes/analysis-2026-10-15.md` | analysis-driven fixes: survive minimize/resize (swapchain rebuild), const and rule-of-five cleanups, Doxygen comments everywhere, `tools/analyze.bat`, `tools/asan.bat`, `.clang-tidy` |

Review guide for Stages 1 to 10: `MyNotes/milestone-helloTriangle.md`.

# Analysis

```
tools\analyze.bat      clang-tidy (+ -Wdocumentation), Cppcheck, lizard
tools\asan.bat         AddressSanitizer build in build-asan\
```
Details: `MyNotes/analysis-2026-10-15.md`.
