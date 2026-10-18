# Milestone: hello triangle (Stages 1 to 10), a review guide

Read this first when you sit down to review. It ties the ten stage notes together, gives a reading order, and lists the questions you should be able to answer at the end.

Companion files:
- `plan.md`: stage list, decisions, GPU check, toolchain.
- `stage01-appStructure.md` ... `stage10-dynamicRendering.md`: one per stage, each with the Vulkan ideas, what the video gets wrong, and a C++ refresher with sources and examples.
- `bugsExplained.md`: all 29 bugs in the author's code, bad vs good, grouped in families.
- `../MYNOTES.md`: the log and build commands.

![the result](images/stage10-triangle.png)

---

## 1. What exists now

```
main thread                                   render thread
-----------                                   -------------
Logger on
GlfwWindow (800x600, size measured in pixels)
RenderThread starts ------------------------> Engine:
                                                instance ............ Stage 2
                                                debug messenger ..... Stage 3 (Debug only)
                                                surface ............. Stage 6
                                                physical device ..... Stage 4
                                                logical device+queue  Stage 5
                                                swapchain + Frames .. Stage 7
                                                image views ......... Stage 8
                                                shader objects ...... Stage 9
                                                command pool+buffers  Stage 10
                                                fence ............... Stage 10
App::run(): wait for window events            loop: engine.draw()   Stage 10
...user closes the window...                    acquire -> submit -> present
RenderThread: done = true, join() <---------- loop ends; waitIdle; deletion
                                                queue destroys everything,
                                                newest first
GlfwWindow closes
```

Destruction order printed at exit (reverse of creation):
```
Goodbye see you!
Deleted command pool
Deleted swapchain          (image views and shaders are destroyed silently in between)
Deleted logical device
Deleted surface
Deleted debug messenger
Deleted instance
```

## 2. Suggested reading order (a couple of hours)

| Step | Read | Then look at | Check yourself |
|---|---|---|---|
| 1 | `stage01-appStructure.md` | `main.cpp`, `render_thread.*` | Why is `done` declared before `thread`? |
| 2 | `stage02-instance.md`, `stage03-validationLayers.md` | `instance.cpp`, `debug_messenger.cpp`, `deletion_queue.h` | What does a layer do, and what does it NOT do? |
| 3 | `stage04-physicalDevice.md`, `stage05-logicalDevice.md` | `device.cpp`, `logical_device.cpp` | Extension vs feature: why both? |
| 4 | `stage06-surface.md`, `stage07-swapchain.md`, `stage08-imageViews.md` | `surface.cpp`, `swapchain.cpp`, `frame.cpp`, `image.cpp` | Who destroys the swapchain images? |
| 5 | `stage09-shaderObjects.md` | `shaders/`, `shader.cpp`, the shader section of `CMakeLists.txt` | Pipeline vs shader object: what moves where? |
| 6 | `stage10-dynamicRendering.md` | `frame.cpp`, `dynamic_state.cpp`, `draw.cpp` | What do the stage mask and the access mask each control? |
| 7 | `bugsExplained.md` by family | the "Bad" snippets in the author's folders | Can you name the family of each bug? |

Tip: watch each video first, then read our stage note: the "What the video gets wrong" table is easier to follow after hearing the claim.

## 3. The ideas to be able to explain

1. **Explicit lifetime.** Every Vulkan object is created and destroyed by you, children before parents. The `DeletionQueue` makes the order automatic and survives a constructor that throws.
2. **Instance -> physical device -> logical device -> everything else.** Plus the surface hanging off the instance.
3. **Extensions and features.** An extension adds functions and types; a feature is a switch you turn on at device creation. Check support first.
4. **Queues and families.** Work goes to queues; families group identical queues. One graphics + present family here.
5. **Swapchain.** A few images owned by the presentation engine; acquire one, draw, present.
6. **Images vs views; layouts and barriers.** Views say how to use an image. Layouts say how it's arranged for a use; barriers change layouts and order work.
7. **Shader objects + dynamic rendering.** No pipeline objects, no render pass objects: state is set with commands, rendering targets are given at `beginRendering`.
8. **Record once, submit many.** Commands are recorded into buffers, then submitted.
9. **The validation layer is a net, not a proof** (bug 22: a spec rule it didn't catch).

## 4. Bugs by family (details in `bugsExplained.md`)

| Family | Entries | One-line lesson |
|---|---|---|
| Lifetime | 1, 9, 25, 29 | cleanup belongs in destructors; wait for the GPU before destroying |
| Silent garbage | 2, 3, 17, 20, 26 | stop at the first failure |
| Threads | 4, 5, 8, 19 | check each function's thread rules; join every thread |
| Design | 6, 13, 24 | one switch per job; constructors construct |
| Performance | 7 | block, don't spin |
| Wrong question | 15, 18 | read the right part of the answer; read field docs |
| Spec rules | 10, 14, 16, 22, 23, 28 | the spec's "must"s apply even when nothing complains |
| Build | 11, 12, 21, 27 | settings for every file at once; no machine-specific paths |

Still present in the author's final Stage 30: bugs 15, 18, 20 (and 22's front face).

## 5. Where we differ from the author on purpose

| Topic | Author | Us | Notes |
|---|---|---|---|
| Errors | `VULKAN_HPP_NO_EXCEPTIONS`, results mostly unchecked | exceptions, caught on the render thread | bug 26 |
| Function lookup | static dispatcher + a second dynamic one | one dynamic dispatcher | `dispatch.cpp` |
| Build | Visual Studio per stage, `compile.bat` | one CMake project, `glslc` at build time | bugs 21, 27 |
| Logging | everything in `Logger` | `logger` (generic) + `vulkan_logging` + `debug_messenger` | one file per topic |
| Function size | some 35+ lines | 25 lines max | your rule |
| Validation | tied to the logger | Debug builds | bug 13 |

## 6. What's measured

| Check | Result |
|---|---|
| Build | Debug and Release, `/W4`, warnings as errors: clean |
| Validation (Debug) | no errors or warnings from our code (3 warnings about GOG Galaxy's layer name) |
| Picture | the triangle, Debug and Release |
| CPU, idle window (Stage 1) | 0.05 s per 3 s (author's version: 3.1 s) |
| CPU, drawing (Stage 10) | about 0.7 core: the render thread waits on the GPU every frame; Stages 12 and 14 fix that |

## 7. What comes next

- **Stage 11**, runtime shader compilation (shaderc, in the SDK).
- **Stage 12**, synchronization: semaphores and fences replace our two CPU waits.
- **STOP before Stage 13**, multithreaded rendering. Review threads, mutexes, atomics, and task systems first; then we go through it line by line. The same channel has a "Multithreading with C++" playlist: https://www.youtube.com/playlist?list=PLn3eTxaOtL2NLlk1rRb_hiZw9uWHxl-3Q
- **Commits**: each stage is saved separately; on your word ("commit") each becomes its own commit, Stage 1 to Stage 10.
