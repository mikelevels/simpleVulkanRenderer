Stage 1

# App structure: a window and a render thread

Video: "Vulkan with C++, Stage 1: App Structure", https://www.youtube.com/watch?v=keH_cPl5SSY
Author's code: `updatedGetIntoGameDev\APIs\vulkan\01 - app structure`
WebGPU counterpart: `MyLearnWebGPU\MyNotes\OpeningaWindow.md` (Step020) for GLFW.

## What this stage does

No Vulkan yet. The author says it himself: "in this first installment I'm actually going to do zero Vulkan". The stage builds the skeleton the whole series hangs on:

```
main thread                         render thread
-----------                         -------------
Logger on
GlfwWindow opens  ------------+
RenderThread starts  ---------+---> Engine built
App::run()                    |     loop until `done`   <- frames go here later
  wait for window events      |
  ...user closes window...    |
App::run() returns            |
RenderThread: done = true ----+---> loop ends, Engine destroyed
              join() waits <-------- thread finishes
GlfwWindow closes
```

Run it: `.\build\Debug\App.exe` (or `Release`). Close the window. Expected output:
```
Made a window called "Simple Vulkan Renderer", 800x600
Made a graphics engine
Window closed.
Goodbye see you!
```
The second and third lines come from different threads.

## Why two threads (the author's reasoning, checked)

From the video, summarized:
- **The window has two users.** The app reads its events (keys, mouse); the renderer draws into it. So it is made first and shared.
- **OpenGL is mostly locked to one thread; Vulkan was designed for many.** True: Vulkan's spec has a whole section on which objects may be used from several threads ("Threading Behavior"), and command buffers can be recorded on many threads at once. The price, as he says, is that *you* synchronize everything. Source: Vulkan spec, "Threading Behavior", https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#fundamentals-threadingbehavior
- **Vulkan records a whole frame of commands, then submits it.** True, and this is the same model as WebGPU's command encoder (`MyLearnWebGPU\MyNotes\TheCommandQueue.md`).
- **Custom memory allocators.** Vulkan's create functions take an optional `VkAllocationCallbacks*` so you can supply your own CPU allocator. True; we'll always pass `nullptr` (the default allocator). Source: spec, "Memory Allocation", https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#memory-allocation
- **"Run threaded code in both Debug and Release."** Good advice: the optimizer reorders code in Release, so some threading bugs appear in only one of them. I ran both.

One thing the video doesn't say: **GLFW's event functions must be called on the main thread** (the one that ran `main()`), which is why the App stays there and the renderer moves. Source: GLFW, "Thread safety", https://www.glfw.org/docs/3.4/intro_guide.html#thread_safety

## The files

| File | Topic | Author's file |
|---|---|---|
| `src/main.cpp` | only `main()`: build the four parts | `src/main.cpp` (also held the thread function) |
| `src/logging/logger.{h,cpp}` | the thread-safe singleton logger | same |
| `src/backend/glfw_backend.{h,cpp}` | `GlfwWindow`: GLFW start, window, cleanup | same (a free function `build_window`) |
| `src/controller/app.{h,cpp}` | the main thread's event loop | same |
| `src/renderer/renderer.{h,cpp}` | `Engine`, empty for now | same |
| `src/renderer/render_thread.{h,cpp}` | `RenderThread`: start, stop, join | was in `main.cpp` |

We keep the author's folder names, file names and `Engine` class name so you can follow his videos side by side.

## C++ refresher (advanced review)

Stage 1 already uses threads. This section covers only what you need to read Stage 1; the full multithreading review comes before Stage 13.

### 1. Stack objects and RAII instead of `new`/`delete`

The author:
```cpp
Engine* engine = new Engine(window);
...
delete engine;
App* app = new App(window);   // never deleted
```
Ours:
```cpp
Engine engine(window);        // destroyed at the closing brace
App app(window);
```
A local variable's destructor runs on every way out of its scope, **including an exception**. Tying a resource (window, thread, file, GPU object) to an object's lifetime is called RAII.

Backup example: a file that always closes.
```cpp
#include <fstream>
void save() {
    std::ofstream file("log.txt");    // opens the file
    file << "hello";
    throw std::runtime_error("oops"); // the file is STILL closed: ~ofstream runs
}
```
Sources: cppreference, RAII, https://en.cppreference.com/w/cpp/language/raii · CG R.5 "Prefer scoped objects, don't heap-allocate unnecessarily", https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rr-scoped

### 2. Destruction order = reverse construction order

Locals in a function, and members in a class, are destroyed in reverse order. Our `main()`:
```cpp
GlfwWindow window(...);              // 1st built, last destroyed
RenderThread renderThread(...);      // 2nd built, 2nd destroyed: stop + join
App app(...);                        // last built, first destroyed
```
So the render thread is always stopped before the window it draws into is destroyed. Members follow the same rule, which is why `RenderThread` declares `done` before `thread` (see `render_thread.h`).

Backup example, members:
```cpp
struct Bad {
    std::thread worker;            // built FIRST: starts reading `flag`...
    std::atomic<bool> flag{false}; // ...which doesn't exist yet
    Bad() : worker([this]{ while (!flag) {} }) {}
};
```
Source: cppreference, "Initialization order" in constructors, https://en.cppreference.com/w/cpp/language/constructor#Initialization_order · GCC and Clang warn when the initializer list is written in a different order (`-Wreorder`); MSVC's equivalent, C5038, is off by default.

### 3. The rule of three/five and `= delete`

A class with a custom destructor (ours destroy windows, stop threads) must also decide what copying means. Usually: forbid it.
```cpp
GlfwWindow a(800, 600, "x");
GlfwWindow b = a;   // with copying allowed: both hold the same pointer,
                    // both destructors destroy it -> crash
```
`GlfwWindow(const GlfwWindow&) = delete;` turns that into a compile error. Source: https://en.cppreference.com/w/cpp/language/rule_of_three

### 4. `std::thread` and `join()`

```cpp
std::thread t(function, arg1, arg2);  // starts running function(arg1, arg2) NOW
...                                    // runs at the same time as this code
t.join();                              // waits here until function returns
```
Rules:
- the arguments are **copied** into the thread; to share something, pass a pointer (we pass `&done`) or `std::ref`;
- a `std::thread` that was started must be `join()`ed (or `detach()`ed) before it is destroyed, **otherwise its destructor calls `std::terminate()`** and the program dies. That's bug 5.
- an exception that escapes the thread's function also calls `std::terminate()`. That's bug 8.

Backup example: C++20's `std::jthread` ("joining thread") does the join for you, and has a built-in stop flag:
```cpp
#include <thread>
std::jthread t([](std::stop_token stop) {
    while (!stop.stop_requested()) { /* work */ }
});
// ~jthread: calls t.request_stop(), then t.join(). No flag, no terminate.
```
We keep the author's `std::atomic<bool>` + `std::thread` because they are the building blocks you'll need in Stage 13; `RenderThread` is our hand-written version of what `jthread` does.
Sources: https://en.cppreference.com/w/cpp/thread/thread · https://en.cppreference.com/w/cpp/thread/thread/~thread · https://en.cppreference.com/w/cpp/thread/jthread · CG CP.25 "Prefer gsl::joining_thread over std::thread", https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rconc-joining_thread

### 5. Data races and `std::atomic<bool>`

If one thread writes a variable while another reads it, and nothing synchronizes them, that's a **data race**, and the C++ standard says the behavior is undefined. In practice, with a plain `bool`, the optimizer may read `done` once and loop forever:
```cpp
bool done = false;                    // plain bool
while (!done) {}                      // compiler may turn this into while(true)
```
`std::atomic<bool>` makes each read and write indivisible and visible to other threads:
```cpp
std::atomic<bool> done{false};
while (!done.load()) {}               // sees the other thread's done = true
```
Sources: cppreference, "Memory model: data races", https://en.cppreference.com/w/cpp/language/memory_model#Threads_and_data_races · https://en.cppreference.com/w/cpp/atomic/atomic

Small detail: the author writes `std::atomic<bool> done = false;`. Before C++17 that didn't compile (atomics can't be copied, and `= false` meant "make a temporary, then copy it"). Since C++17 the copy is skipped by rule ("guaranteed copy elision"), so it compiles. `std::atomic<bool> done{ false };` works in every version.

### 6. `std::mutex` and `std::lock_guard`

An atomic protects one variable. A **mutex** protects a whole block of code: only one thread at a time can hold it.
```cpp
std::lock_guard<std::mutex> lock(printMutex); // lock (or wait for the lock)
std::cout << message << std::endl;            // only one thread at a time here
                                              // unlock at the closing brace
```
`lock_guard` is RAII for a mutex: you can't forget to unlock, even on an exception. Without the mutex, two threads printing at once can interleave: `std::cout << a << b` is two separate calls, and the other thread's text can land between them.
Sources: https://en.cppreference.com/w/cpp/thread/mutex · https://en.cppreference.com/w/cpp/thread/lock_guard · on `cout` from several threads: https://en.cppreference.com/w/cpp/io/cout ("concurrent access ... does not result in a data race, but characters may be interleaved")

### 7. The singleton, done thread-safely

```cpp
Logger& Logger::get_logger() {
    static Logger logger;   // built on the first call, exactly once
    return logger;
}
```
Since C++11, if two threads reach that line together, one builds the object and the other waits. The author's version, `if (!logger) logger = new Logger();`, can run `new` twice if two threads arrive together (it works in his program only because `main()` calls it once before the thread starts), and never deletes it.
Source: cppreference, "Static local variables", https://en.cppreference.com/w/cpp/language/storage_duration#Static_block_variables

### 8. `std::string_view` parameters

The author's `print(std::string message)` copies the text into a new string on every call. `std::string_view` is a (pointer, length) pair that looks at the caller's text without copying. Works with string literals and `std::string` alike.
```cpp
void print(std::string_view message);
print("hello");                // no allocation
std::string s = "world";
print(s);                      // no copy
```
Rule: a `string_view` must not outlive the text it points to; fine for a parameter that is only read during the call. Source: https://en.cppreference.com/w/cpp/string/basic_string_view · CG F.16, https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rf-in

### 9. Anonymous namespace

```cpp
namespace {
void render_thread_main(...) { ... }
}
```
Functions inside are visible only in that `.cpp` file, so another file can't call them, and two files can each have their own helper with the same name without a link error. Source: https://en.cppreference.com/w/cpp/language/namespace#Unnamed_namespaces

### 10. `explicit` constructors

`explicit App(GLFWwindow* window);` stops C++ from silently converting a `GLFWwindow*` into an `App` (for example when passing a pointer to a function that takes an `App`). Make one-argument constructors `explicit` by default. Source: CG C.46, https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rc-explicit

## What we changed from the author's code, and why

| Author | Ours | Why |
|---|---|---|
| `new`/`delete` for App and Engine (App never deleted) | stack objects | RAII, bug 1 |
| `glfwInit()` result ignored | checked, throws | bug 2 |
| window failure printed, `nullptr` returned | throws | bug 3 |
| `new Logger` singleton, unsynchronized `cout` | function-local static, mutex | bug 4 |
| thread + flag + join written out in `main()` | `RenderThread` class (RAII) | bug 5 |
| `App`'s constructor runs the main loop | `App::run()` | bug 6 |
| empty `while (!done) {}`, `glfwPollEvents()` loop | `sleep_for(1ms)`, `glfwWaitEvents()` | bug 7: 3.1 s of CPU per 3 s down to 0.05 s |
| exceptions on the render thread kill the program | caught and logged | bug 8 |
| no `glfwDestroyWindow` | in `~GlfwWindow`, reverse order | habit Vulkan needs |
| `print(std::string)` | `print(std::string_view)` | no copy per call |
| `bool enabled;` with no initial value | `std::atomic<bool> enabled{ false };` | the author's works only because `new Logger()` zero-fills; `Logger logger;` without `()` would leave it random |
| Visual Studio `.sln` + `dependencies/`, C++17 | CMake, GLFW from source, C++20 | portable build; the author moves to C++20 in later stages |
| window "ID Tech 12" | "Simple Vulkan Renderer" | your project |

Every bug is written up in `bugsExplained.md`.
