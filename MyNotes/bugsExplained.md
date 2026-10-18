# Every bug we caught in the author's code, explained

Same format as `updatedMyLearnWebGPU\MyNotes\bugsExplained.md`. Each entry shows:
- **Stage**: where it appears in the author's code (`updatedGetIntoGameDev\APIs\vulkan`);
- **Symptom**: what you see (or would see);
- **Bad**: the author's code;
- **Good**: ours;
- **Why**: the rule, so you can spot it yourself next time.

The bugs fall into a few families. Learn the family and you will spot the next one:

| Family | Entries |
|---|---|
| **Lifetime**: something released too early, too late, never, or twice | 1 |
| **Silent garbage**: errors that are not reported | 2, 3 |
| **Threads**: two threads touching the same thing, or a thread that isn't stopped properly | 4, 5, 8 |
| **Design**: code that works but fights the reader or the next stage | 6 |
| **Performance**: code that is correct but wastes the machine | 7 |

---

## Lifetime

### 1. App and Engine are `new`ed; App is never deleted

**Stage:** 1 (`main.cpp`).

**Symptom:** no visible symptom today; the operating system frees the memory when the program exits. But anything with a destructor that matters (from Stage 2, the Vulkan objects inside `Engine`) is only cleaned up if `delete` is reached, and any exception or early `return` skips it. The validation layer (Stage 3) then reports leaked Vulkan objects.

**Bad:**
```cpp
Engine* engine = new Engine(window);
while (!*done) { }
delete engine;
// ...
App* app = new App(window);   // no delete anywhere
```

**Good** (`render_thread.cpp`, `main.cpp`):
```cpp
Engine engine(window);        // destroyed at the closing brace, always
App app(window.get());
```

**Why:** use a local object unless the object must outlive its scope; if it must, hold it in `std::unique_ptr`. Source: CG R.5 and R.11 ("Avoid calling new and delete explicitly"), https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rr-newdelete

---

## Silent garbage

### 2. `glfwInit()` can fail, and nobody checks

**Stage:** 1 onward (`glfw_backend.cpp`).

**Symptom:** where GLFW can't start (for example, a remote session with no display), every later GLFW call fails on its own, and the first message you see is about the window, not the real cause.

**Bad:**
```cpp
glfwInit();
```

**Good** (`glfw_backend.cpp`):
```cpp
if (glfwInit() != GLFW_TRUE) {
    throw std::runtime_error("glfwInit failed");
}
```

**Why:** check every call that can fail, where it's called. Source: GLFW reference, `glfwInit` returns "GLFW_TRUE if successful, or GLFW_FALSE if an error occurred", https://www.glfw.org/docs/3.4/group__init.html

### 3. Window creation failure is printed, then ignored

**Stage:** 1 onward.

**Symptom:** `build_window` returns `nullptr`, `main()` hands it to both threads, and `glfwWindowShouldClose(nullptr)` is undefined behavior (in practice a crash, far from the cause). From Stage 6, surface creation fails with a confusing Vulkan error instead.

**Bad:**
```cpp
GLFWwindow* window = glfwCreateWindow(width, height, name, nullptr, nullptr);
if (window) { /* print success */ }
else { logger->print("GLFW window creation failed."); }
return window;   // may be nullptr; nobody checks
```

**Good** (`glfw_backend.cpp`):
```cpp
window = glfwCreateWindow(width, height, name, nullptr, nullptr);
if (window == nullptr) {
    glfwTerminate();   // still in the constructor: the destructor won't run
    throw std::runtime_error("glfwCreateWindow failed");
}
```

**Why:** a failure that only prints moves the crash somewhere else. Stop at the first failure. Source: CG E.2 "Throw an exception to signal that a function can't perform its assigned task", https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Re-throw

---

## Threads

### 4. The logger singleton isn't thread-safe

**Stage:** 1 onward (`logger.cpp`). Used by both threads from Stage 1.

**Symptom:** two problems, both rare, both timing-dependent (the worst kind to debug):
- if two threads call `get_logger()` for the first time together, both can see `logger == nullptr` and both run `new Logger()`. It doesn't happen in Stage 1 only because `main()` calls it once before starting the thread.
- two threads printing at once can interleave: `std::cout << message << std::endl` is two calls, and the other thread's line can land between the text and the newline.

**Bad:**
```cpp
Logger* Logger::get_logger() {
    if (!logger) {
        logger = new Logger();   // check-then-act: two threads can both pass the check
    }
    return logger;
}
void Logger::print(std::string message) {
    if (!enabled) return;
    std::cout << message << std::endl;   // no lock
}
```

**Good** (`logger.cpp`):
```cpp
Logger& Logger::get_logger() {
    static Logger logger;   // C++11: initialized exactly once, thread-safe
    return logger;
}
void Logger::print(std::string_view message) {
    if (!enabled) return;
    std::lock_guard<std::mutex> lock(printMutex);
    std::cout << message << std::endl;
}
```

**Why:** "check, then act" on shared data is a race unless something makes the two steps one (a lock, an atomic, or the language's static-initialization guarantee). Sources: cppreference, static block variables, https://en.cppreference.com/w/cpp/language/storage_duration#Static_block_variables · CG CP.2 "Avoid data races", https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rconc-races

### 5. An exception before `join()` kills the program

**Stage:** 1 (`main.cpp`), and every stage that keeps this `main()`.

**Symptom:** if `new App(window)` (or anything between starting the thread and joining it) throws, `main()` unwinds, the `std::thread` object is destroyed while still joinable, and its destructor calls `std::terminate()`: the program dies with no message, and the render thread never hears `done`.

**Bad:**
```cpp
std::atomic<bool> done = false;
std::thread render_thread(spawn_render_thread, window, &done);
App* app = new App(window);   // if this throws...
done = true;                  // ...these two lines are skipped
render_thread.join();
```

**Good** (`render_thread.cpp`):
```cpp
RenderThread::~RenderThread() {
    done = true;     // runs on every way out of main(), exceptions included
    thread.join();
}
```

**Why:** "stop and join" is cleanup, and cleanup belongs in a destructor. C++20's `std::jthread` does exactly this. Sources: cppreference, `std::thread::~thread` ("If *this has an associated thread (joinable() == true), std::terminate() is called"), https://en.cppreference.com/w/cpp/thread/thread/~thread · CG CP.25, https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rconc-joining_thread

### 8. An exception on the render thread kills the program silently

**Stage:** 1 onward (`spawn_render_thread`). Matters from Stage 2, when `Engine`'s constructor starts calling Vulkan functions that throw `vk::SystemError`.

**Symptom:** the program vanishes, with no message about what failed.

**Bad:**
```cpp
void spawn_render_thread(GLFWwindow* window, std::atomic<bool>* done) {
    Engine* engine = new Engine(window);   // a throw here escapes the thread
    ...
}
```

**Good** (`render_thread.cpp`):
```cpp
try {
    Engine engine(window);
    while (!done->load()) { ... }
}
catch (const std::exception& error) {
    Logger::get_logger().print(error.what());
}
```

**Why:** an exception that leaves a thread's top function calls `std::terminate()`; exceptions don't travel to the thread that called `join()`. Catch at the top of every thread. (To hand the error to another thread, see `std::exception_ptr`, https://en.cppreference.com/w/cpp/error/exception_ptr.) Source: cppreference, `std::thread` constructor, "If the function throws an exception, std::terminate is called", https://en.cppreference.com/w/cpp/thread/thread/thread


---

## Design

### 6. `App`'s constructor runs the whole program

**Stage:** 1 (`app.cpp`).

**Symptom:** `App* app = new App(window);` doesn't return until the window is closed, so `app` only gets its value when the app is already finished. Anyone reading `main()` thinks the program ends right after creating the app. And while a constructor runs, the object isn't "fully built" by the language's rules, so later code that calls a virtual function from the loop, or keeps `this` somewhere, behaves surprisingly.

**Bad:**
```cpp
App::App(GLFWwindow* window) : window(window) {
    logger = Logger::get_logger();
    main_loop();   // runs until the window closes
}
```

**Good** (`app.cpp`, `main.cpp`):
```cpp
App app(window.get());   // only stores the window
app.run();               // the loop, called by name
```

**Why:** a constructor's job is to make the object valid; doing the work is a separate, named step. Source: CG C.41 "A constructor should create a fully initialized object" (and nothing more), https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rc-complete

---

## Performance

### 7. Two loops spin the CPU while doing nothing

**Stage:** 1 (`main.cpp`'s render loop, `app.cpp`'s event loop). The render loop fills up with real work in later stages; the event loop stays like this.

**Symptom:** measured on your PC, each program left open for 3 seconds:

| Program | CPU time used in 3 s |
|---|---|
| author's Stage 1 (Release) | **3.1 s** (about one full core) |
| ours (Release) | 0.05 s |

A laptop's fan spins up and its battery drains with an empty window open.

**Bad:**
```cpp
while (!*done) {
    //Do work repeatedly!       <- nothing to do: loops millions of times a second
}
// and on the main thread:
while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();         // returns immediately when there are no events
}
```

**Good** (`render_thread.cpp`, `app.cpp`):
```cpp
while (!done->load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));   // placeholder until frames exist
}
// main thread:
while (!glfwWindowShouldClose(window)) {
    glfwWaitEvents();         // sleeps until an event arrives
}
```

**Why:** a loop with nothing to do should block (wait for an event, a condition variable, a fence) instead of polling. `glfwPollEvents` is right when the same loop also draws frames; here drawing lives on the other thread. Source: GLFW, "Event processing", https://www.glfw.org/docs/3.4/input_guide.html#events
