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
