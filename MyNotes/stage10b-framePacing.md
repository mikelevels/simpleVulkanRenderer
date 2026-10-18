Stage 10b (a lesson of ours, not one of the author's videos)

# Frame pacing: measure first, then cap the frame rate

Why this lesson exists: the 2026-10-15 analysis sweep found the renderer keeping **0.8 of a CPU core** busy to draw one triangle, while the WebGPU apps used 0.03. You asked for any major bottleneck to become a tutorial step. This is it.

Files: `src/renderer/frame_timer.{h,cpp}` (new), `swapchain.h` (`preferredPresentMode`), `swapchain.cpp` (`choose_present_mode`), `render_thread.cpp` (calls `timer.tick()`), `CMakeLists.txt` (option `SVR_FORCE_PRESENT_MODE`).

## 1. Measure before you change anything

A "bottleneck" claim needs a number. `FrameTimer` counts frames and every two seconds logs:
```
59.9 frames per second, 16.7 ms per frame
```
It's 20 lines: `tick()` adds one, and when two seconds of `std::chrono::steady_clock` have passed it prints frames ÷ seconds and resets.

## 2. The experiment

Same Release build, three present modes forced with the CMake option, 5 seconds each after start-up, measured on your PC (TITAN Xp, 60 Hz monitor):

| Present mode | Frames per second | CPU cores busy | GPU busy | Frames you can see |
|---|---|---|---|---|
| Mailbox (the author's Stage 10 choice) | **5,880** | **0.78** | 10 % | 60 |
| Immediate (the author's later choice) | 6,870 | 0.69 | 10-17 % | 60 (with tearing) |
| **FIFO (ours now)** | **59.9** | **0.03** | 2 % | 60 |

The second sweep's profiler (AMD uProf) adds the *where*: in the Mailbox build, 62 % of the app's CPU time was inside the NVIDIA driver, i.e. spent submitting and waiting for frames nobody sees.

How to repeat it yourself:
```
cmake -B build-mailbox -DSVR_FORCE_PRESENT_MODE=Mailbox
cmake --build build-mailbox --config Release
.\build-mailbox\Release\App.exe
```
(also `Immediate` or `Fifo`; leave the option empty for the normal choice).

## 3. What the numbers mean

With **Mailbox** the swapchain never makes us wait. The render loop draws, presents, and immediately draws again; whenever a newer image is ready it replaces the one waiting for the screen. The monitor still shows 60 images a second, so **about 97 of every 98 frames are drawn and thrown away** (5,880 drawn, 60 shown). Each of those frames costs a full acquire, submit, `waitIdle` and present on the CPU: that's the 0.78 core.

With **FIFO** the swapchain is a queue in front of the monitor. When it's full, `vkAcquireNextImageKHR` / `vkQueuePresentKHR` block until the monitor has taken an image (v-sync). The render thread sleeps inside the driver between frames, so the CPU is nearly idle and the frame rate equals the refresh rate.

| | FIFO | Mailbox | Immediate |
|---|---|---|---|
| Waits for the monitor | yes | no | no |
| Tearing | no | no | yes |
| Input-to-screen delay | up to ~2 frames | ~1 frame | lowest |
| Wasted frames | none | most | most |
| Always available | **yes** (the spec requires it) | no | no |

Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#VkPresentModeKHR

## 4. The fix, and why it's a default rather than a rule

`swapchain.h` now has one named setting:
```cpp
inline constexpr vk::PresentModeKHR preferredPresentMode = vk::PresentModeKHR::eFifo;
```
and `choose_present_mode` returns it if the surface offers it, otherwise FIFO. Result: 60 fps at 0.03 core, like the WebGPU apps.

The author's Mailbox isn't *wrong*: it's what you want when measuring how fast the renderer *could* go, or in a game where input delay matters more than power. That's why the CMake option exists. But a renderer that heats a laptop drawing invisible frames is a poor default, and FIFO is the only mode the spec guarantees.

What FIFO does NOT fix: the CPU still waits for the GPU twice per frame (`waitForFences` after acquire, `waitIdle` after submit). That's the next bottleneck, and the author fixes it in **Stage 12** (semaphores) and **Stage 14** (frames in flight). With FIFO you just can't see it yet, because the monitor is now the slowest part.

## 5. C++ refresher

### `std::chrono`
```cpp
using Clock = std::chrono::steady_clock;
const Clock::time_point now = Clock::now();
const std::chrono::duration<double> elapsed = now - windowStart;   // seconds, as a double
elapsed.count();                                                   // the number itself
```
- `steady_clock` only moves forward; `system_clock` (the wall clock) can jump when the time is synchronized, which would break a timer.
- `time_point - time_point = duration`; `duration<double>` means "seconds stored in a double", so `count()` gives e.g. `2.0134`.

Backup example, timing any piece of code:
```cpp
const auto start = std::chrono::steady_clock::now();
do_work();
const auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
std::cout << "took " << ms << " ms\n";
```
Source: https://en.cppreference.com/w/cpp/chrono/steady_clock · https://en.cppreference.com/w/cpp/chrono/duration

### `std::find` and the conditional operator
```cpp
const bool offered = std::find(presentModes.begin(), presentModes.end(), preferredPresentMode)
    != presentModes.end();
return offered ? preferredPresentMode : vk::PresentModeKHR::eFifo;
```
`std::find` returns an iterator to the first match, or `end()` if there's none. `a ? b : c` is an expression: `b` if `a` is true, else `c`. Sources: https://en.cppreference.com/w/cpp/algorithm/find · https://en.cppreference.com/w/cpp/language/operator_other#Conditional_operator

### A preprocessor switch from CMake
```cmake
target_compile_definitions(App PRIVATE SVR_FORCE_PRESENT_MODE=e${SVR_FORCE_PRESENT_MODE})
```
```cpp
#ifdef SVR_FORCE_PRESENT_MODE
    vk::PresentModeKHR::SVR_FORCE_PRESENT_MODE;   // becomes vk::PresentModeKHR::eMailbox
#else
    vk::PresentModeKHR::eFifo;
#endif
```
The macro is pasted in as text before compiling, so `-DSVR_FORCE_PRESENT_MODE=Mailbox` turns into the enum value `eMailbox`. Useful for experiments; for anything a user should change while running, use a real setting instead.

## 6. Try it yourself

1. Run the three builds above and watch Task Manager's CPU column and the "frames per second" lines.
2. In the Mailbox build, set a breakpoint in `Engine::draw` and count how often it hits in one second (you'll give up quickly, which is the point).
3. Predict: what happens to the FIFO frame rate on a 144 Hz monitor? (It becomes 144: FIFO follows the monitor, not a fixed number.)
