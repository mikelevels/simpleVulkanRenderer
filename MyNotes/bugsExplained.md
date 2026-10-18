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
| **Lifetime**: something released too early, too late, never, or twice; or not rebuilt when it must be | 1, 9, 25, 29, 30 |
| **Silent garbage**: errors that are not reported | 2, 3, 17, 20, 26 |
| **Threads**: two threads touching the same thing, or a thread that isn't stopped properly | 4, 5, 8, 19 |
| **Design**: code that works but fights the reader or the next stage | 6, 13, 24 |
| **Performance**: code that is correct but wastes the machine | 7, 31 |
| **Wrong question**: the code asks Vulkan something, then reads the wrong part of the answer | 15, 18 |
| **Spec rules**: code that breaks a "must" in the Vulkan spec, whether or not anything complains | 10, 14, 16, 22, 23, 28 |
| **Build**: code that only builds or runs on the author's machine | 11, 12, 21, 27 |

Entries 1 to 8 are Stage 1. Entries 9 to 29 are Stages 2 to 10; their bad code is from the author's `finished` folder of the stage named. Entries 30 and 31 were found by the 2026-10-15 analysis sweep, in our Stage 10 as well as the author's.

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

---

# Stages 2 to 10

## Lifetime

### 9. A constructor that throws leaks every Vulkan object made so far

**Stage:** 2 onward (`renderer.cpp`, the deletion queues).

**Symptom:** none while everything works. As soon as one step of the Engine's constructor fails (no GPU supports shader objects, a shader file is missing), the objects created before it (instance, messenger, surface, device...) are never destroyed, because `~Engine()` never runs for an object whose constructor didn't finish. The validation layer then reports leaked objects at exit.

**Bad:**
```cpp
Engine::~Engine() {
    while (deviceDeletionQueue.size() > 0) { deviceDeletionQueue.back()(logicalDevice); deviceDeletionQueue.pop_back(); }
    while (instanceDeletionQueue.size() > 0) { instanceDeletionQueue.back()(instance); instanceDeletionQueue.pop_back(); }
}
```

**Good** (`deletion_queue.h`):
```cpp
class DeletionQueue {
public:
    ~DeletionQueue() { flush(); }   // a MEMBER's destructor runs even if
    ...                             // the Engine's constructor throws
};
```

**Why:** when a constructor throws, C++ destroys the members that were already built, but doesn't call the class's own destructor. Put cleanup in members (RAII), not in `~Engine()`. Source: cppreference, "Constructors and member initializer lists" (exceptions during construction), https://en.cppreference.com/w/cpp/language/constructor · CG C.31/E.6, https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Re-raii

### 25. `malloc`ed name arrays leak on every early return

**Stage:** 3 onward (`instance.cpp`, `device.cpp`).

**Symptom:** a few bytes lost each time instance creation fails. Small, but it's the pattern that matters: every `return` between `malloc` and `free` is a leak.

**Bad:**
```cpp
const char** ppEnabledExtensionNames = (const char**)malloc(enabledExtensionCount * sizeof(char*));
...
if (!supported_by_instance(...)) {
    return nullptr;                    // never freed
}
...
free(ppEnabledExtensionNames);
```

**Good** (`instance.cpp`):
```cpp
std::vector<const char*> extensions = required_extensions();   // frees itself
vk::InstanceCreateInfo createInfo({}, &appInfo, layers, extensions);
```

**Why:** `std::vector` is RAII for arrays, and Vulkan-Hpp's create-info constructors accept a vector directly (through `vk::ArrayProxy`), setting both the count and the pointer. Source: CG R.10 "Avoid malloc() and free()", https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rr-mallocfree

### 29. Objects destroyed while the GPU may still use them

**Stage:** 10 (`~Engine`).

**Symptom:** none today, because the author's `draw()` ends with `waitIdle()`. Any later change that lets the GPU run ahead (Stage 12 does exactly that) makes the destructor destroy command buffers and images the GPU is still reading: validation errors at exit, or a crash.

**Bad:**
```cpp
Engine::~Engine() {
    logger->print("Goodbye see you!");
    while (deviceDeletionQueue.size() > 0) { ... }   // straight to destroying
}
```

**Good** (`renderer.cpp`):
```cpp
Engine::~Engine() {
    try {
        if (logicalDevice) { logicalDevice.waitIdle(); }
    }
    catch (const std::exception& error) { ... }      // destructors must not throw
}
```

**Why:** the spec requires that an object isn't in use by the GPU when it is destroyed ("all submitted commands that refer to X must have completed execution"). `vkDeviceWaitIdle` before teardown is the simple, always-correct way. Source: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#vkDestroyDevice

---

## Silent garbage

### 17. `glfwCreateWindowSurface`'s result is ignored

**Stage:** 6 onward (`renderer.cpp`).

**Symptom:** if surface creation fails, `raw_surface` is an uninitialized variable: the engine carries on with a random handle and crashes later in the swapchain code, far from the cause.

**Bad:**
```cpp
VkSurfaceKHR raw_surface;                                     // uninitialized
glfwCreateWindowSurface(instance, window, nullptr, &raw_surface);
surface = raw_surface;
```

**Good** (`surface.cpp`):
```cpp
VkSurfaceKHR rawSurface = VK_NULL_HANDLE;
VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &rawSurface);
if (result != VK_SUCCESS) {
    throw vk::SystemError(vk::make_error_code(static_cast<vk::Result>(result)), "glfwCreateWindowSurface");
}
```

**Why:** check every `VkResult`. Initialize every variable. Source: GLFW, "Creating the surface", https://www.glfw.org/docs/3.4/vulkan_guide.html#vulkan_surface · CG ES.20 "Always initialize an object", https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Res-always

### 20. `read_file` carries on after failing to open the file

**Stage:** 9 onward (`file.cpp`). Still present in the author's Stage 30.

**Symptom:** run the program from a different folder and the shaders aren't found. Instead of "Failed to load shaders/vertex.spv" you get a `std::length_error` or `bad_alloc` from deep inside `std::vector`, because the size is garbage.

**Bad:**
```cpp
std::ifstream file(filename, std::ios::ate | std::ios::binary);
if (!file.is_open()) {
    logger->print(line);              // ...and continue
}
size_t filesize{ static_cast<size_t>(file.tellg()) };   // tellg() == -1 -> 18446744073709551615
std::vector<char> buffer(filesize);
```

**Good** (`file.cpp`):
```cpp
if (!file.is_open()) {
    throw std::runtime_error(std::string("Failed to open ") + filename);
}
```

**Why:** `tellg()` returns `-1` on a failed stream; converting `-1` to the unsigned `size_t` wraps around to the largest possible value. Source: https://en.cppreference.com/w/cpp/io/basic_istream/tellg · unsigned wrap-around: https://en.cppreference.com/w/cpp/language/implicit_conversion#Integral_conversions

### 26. Failures return `nullptr`, and the caller carries on

**Stage:** 2 onward (`make_instance`, `choose_physical_device`, `create_logical_device`, `make_command_pool`, `make_shader_objects`...).

**Symptom:** the first failure is printed, then the next function receives a null handle and crashes (Vulkan does no argument checking without the validation layer). The crash points at the wrong function.

**Bad:**
```cpp
vk::ResultValue<vk::Instance> instanceAttempt = vk::createInstance(createInfo);
if (instanceAttempt.result != vk::Result::eSuccess) {
    logger->print("Failed to create Instance!");
    return nullptr;                  // the Engine uses this "instance" next
}
```

**Good** (everywhere): we leave Vulkan-Hpp's exceptions on (no `VULKAN_HPP_NO_EXCEPTIONS`), so a failed call throws `vk::SystemError` with the error's name, and our own checks throw `std::runtime_error`. The render thread catches and prints it (entry 8).
```cpp
vk::Instance instance = vk::createInstance(createInfo);   // throws on failure
```

**Why:** the author turns exceptions off "for overhead", but then has to check every result by hand, and mostly doesn't. With table-based exception handling (MSVC x64, GCC, Clang) the success path costs nothing; you pay only when something fails. Source: Vulkan-Hpp README, "Return values, Error Codes & Exceptions", https://github.com/KhronosGroup/Vulkan-Hpp#return-values-error-codes--exceptions

---

## Threads

### 19. GLFW window size asked from the render thread, in the wrong units

**Stage:** 7 onward (`renderer.cpp`).

**Symptom:** two problems. (a) GLFW says `glfwGetWindowSize` "must only be called from the main thread"; the Engine runs on the render thread. It works today on Windows by luck. (b) It returns *screen coordinates*; on a high-DPI screen (a 4K laptop at 200%) the window has twice as many *pixels*, and the swapchain needs pixels. On Windows `currentExtent` is always set, so the value isn't used yet; on other systems it would make a blurry, half-size image.

**Bad:**
```cpp
int width, height;
glfwGetWindowSize(window, &width, &height);   // on the render thread; screen coordinates
swapchain.build(logicalDevice, physicalDevice, surface, width, height, deviceDeletionQueue);
```

**Good** (`glfw_backend.cpp`, `render_thread.cpp`):
```cpp
// main thread, right after creating the window:
glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);   // pixels
// RenderThread's constructor (still the main thread) copies them into the new thread:
thread(render_thread_main, window.get(), window.framebuffer_width(), window.framebuffer_height(), &done)
```

**Why:** check each library function's "thread safety" note before calling it from another thread. Sources: GLFW `glfwGetWindowSize` reference, https://www.glfw.org/docs/3.4/group__window.html#gaeea7cbc03373a41fb51cfbf9f2a5d4c6 · "Window size vs framebuffer size", https://www.glfw.org/docs/3.4/window_guide.html#window_fbsize

---

## Design

### 13. Turning off the log also turns off validation

**Stage:** 3 onward (`instance.cpp`, `device.cpp`).

**Symptom:** `logger->set_mode(false)` (to get a quieter console) silently removes the validation layer, so real errors go unreported.

**Bad:**
```cpp
if (logger->is_enabled()) {
    enabledLayerCount++;   // the validation layer
}
```

**Good** (`instance.h`):
```cpp
#ifdef NDEBUG
inline constexpr bool enableValidation = false;   // Release
#else
inline constexpr bool enableValidation = true;    // Debug
#endif
```

**Why:** one switch, one job. Validation is about build type (Debug vs Release), not about how chatty the console is. `NDEBUG` is the standard macro for that (it also disables `assert`). Source: https://en.cppreference.com/w/cpp/error/assert

### 24. `set_mode(true)` forces the log back on

**Stage:** 5 to 29 (`renderer.cpp`).

**Symptom:** `main()` turns the logger off; the Engine turns it back on halfway through construction. Combined with entry 13, the device is then created with validation layer settings that disagree with the instance's.

**Bad:**
```cpp
logger->set_mode(false);
physicalDevice = choose_physical_device(instance);
logger->set_mode(true);   // was it on before? doesn't matter, it is now
```

**Good:** we never change the mode inside the engine. If you want one quiet section, save and restore:
```cpp
bool wasEnabled = logger.is_enabled();
logger.set_mode(false);
// ...
logger.set_mode(wasEnabled);
```

**Why:** code that changes a global setting must put it back the way it found it. (An RAII "guard" object that restores in its destructor is the C++ way; `std::lock_guard` is one.)

---

## Wrong question

### 15. "Can this queue family present?" is answered by whether the call succeeded

**Stage:** 6 onward (`device.cpp`). Still present in the author's Stage 30.

**Symptom:** every queue family is treated as able to present to the window. On your TITAN Xp, family 0 can, so nothing shows. On a GPU where the first graphics family can't present, the program would pick it and fail at present time.

**Bad:**
```cpp
if (physicalDevice.getSurfaceSupportKHR(i, surface).result == vk::Result::eSuccess) {
    canPresent = true;   // eSuccess means "I answered", not "yes"
}
```

**Good** (`logical_device.cpp`):
```cpp
bool canPresent = !surface || physicalDevice.getSurfaceSupportKHR(i, surface);   // the Bool32 answer
```

**Why:** `vkGetPhysicalDeviceSurfaceSupportKHR` returns a `VkResult` (did the query work?) AND writes a `VkBool32` (is presenting supported?). They are different questions. Source: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#vkGetPhysicalDeviceSurfaceSupportKHR

### 18. `maxImageCount == 0` means "no limit", not "zero"

**Stage:** 7 onward (`swapchain.cpp`). Still present in the author's Stage 30.

**Symptom:** on a platform that reports no maximum (common on Linux/Wayland and some Android drivers), `std::min(0, min + 1)` asks for 0 images: swapchain creation fails. Your PC reports a maximum of 8, so it works here.

**Bad:**
```cpp
imageCount = std::min(support.capabilities.maxImageCount, support.capabilities.minImageCount + 1);
```

**Good** (`swapchain.cpp`):
```cpp
std::uint32_t count = capabilities.minImageCount + 1;
if (capabilities.maxImageCount > 0) {
    count = std::min(count, capabilities.maxImageCount);
}
```

**Why:** read the spec's description of every field you use; "0 means unlimited" is a common convention. Source: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#VkSurfaceCapabilitiesKHR ("maxImageCount ... A value of 0 means that there is no limit")

---

## Spec rules

### 10. The instance asks for "whatever the loader has"

**Stage:** 2 onward (`instance.cpp`).

**Symptom:** on your PC the program asks for Vulkan 1.4 (the loader's version), while it's compiled against 1.3 headers and uses 1.3 features. It works, but the request doesn't describe the program.

**Bad:**
```cpp
uint32_t version = vk::enumerateInstanceVersion().value;
version &= ~(0xFFFU);                    // clear the patch number
vk::ApplicationInfo appInfo(applicationName, version, "Doing it the hard way", version, version);
```

**Good** (`instance.cpp`):
```cpp
constexpr std::uint32_t requiredApiVersion = VK_API_VERSION_1_3;
// ... check the loader is at least 1.3, then:
vk::ApplicationInfo appInfo(applicationName, VK_MAKE_API_VERSION(0, 0, 1, 0),
    "Simple Vulkan Renderer", VK_MAKE_API_VERSION(0, 0, 1, 0), requiredApiVersion);
```

**Why:** the spec defines `apiVersion` as "the highest version of Vulkan that the application is designed to use". The app's and engine's own versions are separate numbers, not the API version. Source: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#VkApplicationInfo

### 14. The GPU is chosen without checking Vulkan 1.3 or the features we enable

**Stage:** 4 (version), 9 and 10 (features), `device.cpp`.

**Symptom:** on a GPU with the extension names but without the features (or older than 1.3), `createDevice` fails with `VK_ERROR_FEATURE_NOT_PRESENT`, or the program picks the wrong GPU on a laptop with two.

**Bad:**
```cpp
vk::PhysicalDeviceShaderObjectFeaturesEXT shaderFeatures = vk::PhysicalDeviceShaderObjectFeaturesEXT(1);   // turned on, never checked
```

**Good** (`device.cpp`):
```cpp
auto chain = device.getFeatures2<vk::PhysicalDeviceFeatures2,
    vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceShaderObjectFeaturesEXT>();
if (!chain.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering) { return false; }
if (!chain.get<vk::PhysicalDeviceShaderObjectFeaturesEXT>().shaderObject) { return false; }
```
plus `apiVersion >= VK_API_VERSION_1_3`, plus preferring a discrete GPU.

**Why:** "If any feature in pEnabledFeatures is not supported by the physical device, vkCreateDevice must fail" (`VK_ERROR_FEATURE_NOT_PRESENT`). Query before you enable. Source: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#features

### 16. Device layers: deprecated, and counted wrong

**Stage:** 5 onward (`device.cpp`; the count bug is fixed in the author's Stage 30).

**Symptom:** with the logger off, the device is created with `enabledLayerCount = 1` and a null list of names: undefined behavior (the driver reads through a null pointer). With it on, the layer names are simply ignored.

**Bad:**
```cpp
uint32_t enabled_layer_count = 1;            // 1 even when...
const char** ppEnabledLayers = nullptr;      // ...this stays null
if (logger->is_enabled()) { ... }
```

**Good** (`logical_device.cpp`): no device layers at all.
```cpp
vk::DeviceCreateInfo deviceInfo({}, queueInfo, {}, extensions);   // {} = no layers
```

**Why:** device layers were deprecated in Vulkan 1.0.13; the instance's layers apply to every device. A count and its pointer must always agree. Source: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#extendingvulkan-layers-devicelayerdeprecation

### 22. Two pieces of drawing state are never set

**Stage:** 10 (`frame.cpp`). `vkCmdSetVertexInputEXT` is added in the author's Stage 14; `vkCmdSetFrontFace` never.

**Symptom:** nothing visible on your PC, and (we tested) our SDK's validation layer doesn't report it either, though its rule text is inside the layer. On another driver, a draw with unset state is undefined behavior: anything from working to garbage.

**Bad:** the author's `annoying_boilerplate_...` sets 17 states but not these two.

**Good** (`dynamic_state.cpp`):
```cpp
cmd.setFrontFace(vk::FrontFace::eClockwise);   // required while rasterization is on
cmd.setVertexInputEXT(nullptr, nullptr);       // required when a vertex shader object is bound
```

**Why:** with shader objects, the spec lists exactly which `vkCmdSet*` calls must happen before a draw (VUID-vkCmdDraw-None-08628 for front face, VUID-vkCmdDraw-None-04914 for vertex input). The validation layer catches most of them, not all: it is a safety net, not a proof. Source: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#shaders-objects-state

### 23. Presenting an image that was never acquired

**Stage:** 10 (`renderer.cpp`); the author fixes it in Stage 12.

**Symptom:** it shows a triangle on your NVIDIA driver, by luck. The program always submits frame 0's commands and presents image 0 without asking the swapchain whether image 0 is free. The validation layer has a rule for exactly this (`VUID-VkPresentInfoKHR-pImageIndices-01430`).

**Bad:**
```cpp
void Engine::draw() {
    uint32_t imageIndex{ 0 };                        // never acquired
    submitInfo.pCommandBuffers = &frames[0].commandBuffer;
    graphicsQueue.submit(submitInfo);
    graphicsQueue.waitIdle();
    presentInfo.pImageIndices = &imageIndex;
    graphicsQueue.presentKHR(presentInfo);
    graphicsQueue.waitIdle();
}
```

**Good** (`draw.cpp`):
```cpp
vk::ResultValue<std::uint32_t> acquired = logicalDevice.acquireNextImageKHR(
    swapchain.chain, noTimeout, nullptr, imageAcquiredFence);
logicalDevice.waitForFences(imageAcquiredFence, VK_TRUE, noTimeout);   // image really free
logicalDevice.resetFences(imageAcquiredFence);
// submit frames[acquired.value].commandBuffer, waitIdle, present acquired.value
```

**Why:** "Each element of pImageIndices must be the index of a presentable image acquired from the swapchain". The swapchain decides which image is yours. Source: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#VUID-VkPresentInfoKHR-pImageIndices-01430

### 28. SPIR-V read into `std::vector<char>`

**Stage:** 9 onward (`shader.cpp`). Same in ours, explained instead of changed.

**Symptom:** none on Windows. SPIR-V is a list of 32-bit words and `pCode` must be 4-byte aligned. `std::vector<char>`'s buffer comes from `operator new`, which on every desktop compiler is aligned to at least 8 or 16 bytes, so it works, but the type itself promises only 1-byte alignment.

**Better:** read into `std::vector<std::uint32_t>` (size divided by 4), which states the requirement in the type. We'll make that change when Stage 11 replaces the file loading with runtime compilation.

**Why:** source: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#VUID-VkShaderCreateInfoEXT-pCode-08493

---

## Build

### 11. `VULKAN_HPP_NO_EXCEPTIONS` is set in some headers but not others

**Stage:** 2 onward.

**Symptom:** `logger.h` includes `vulkan.hpp` without the macro, `instance.h` and friends define it first. Files that include `logger.h` first compile Vulkan-Hpp WITH exceptions; the others WITHOUT. The same inline functions then exist in two different versions in one program, a violation of the One Definition Rule (undefined behavior, and no compiler or linker is required to tell you). The author's `logger.cpp` only compiles *because* it got the exceptions version.

**Bad:**
```cpp
// instance.h                          // logger.h
#define VULKAN_HPP_NO_EXCEPTIONS       #include <vulkan/vulkan.hpp>   // no define
#include <vulkan/vulkan.hpp>
```

**Good** (`CMakeLists.txt`): settings for a library go to every file at once.
```cmake
target_compile_definitions(App PRIVATE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
```

**Why:** configuration macros must be identical in every translation unit. Source: cppreference, "One Definition Rule", https://en.cppreference.com/w/cpp/language/definition#One_Definition_Rule · Vulkan-Hpp README, "Configuration Options", https://github.com/KhronosGroup/Vulkan-Hpp#configuration-options

### 12. Code that only compiles with a newer Vulkan SDK than the one installed

**Stage:** 3 onward (`vk::detail::DispatchLoaderDynamic`, `vk::PFN_DebugUtilsMessengerCallbackEXT`).

**Symptom:** with your SDK (1.3.283) the author's code doesn't compile: `detail` is not a member of `vk` for that class, and there is no `vk::PFN_DebugUtilsMessengerCallbackEXT`. These names come from a newer Vulkan-Hpp. On top of that, the author's `reinterpret_cast` of the callback would hide a WRONG signature too, turning a compile error into a crash.

**Bad:**
```cpp
reinterpret_cast<vk::PFN_DebugUtilsMessengerCallbackEXT>(debugCallback)
```

**Good** (`debug_messenger.cpp`): give the callback exactly the C signature and pass it without a cast; use the default dynamic dispatcher, which exists in every version (`dispatch.cpp`).
```cpp
VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* data, void*);
...
vk::DebugUtilsMessengerCreateInfoEXT({}, severities, types, &debug_callback);
```

**Why:** a `reinterpret_cast` between function pointer types tells the compiler "trust me"; calling through the wrong type is undefined behavior. When you follow a tutorial, write down which SDK version it was made with (our `plan.md` does), because headers change between versions. Source: https://en.cppreference.com/w/cpp/language/reinterpret_cast

### 21. Shaders found only when started from the project folder

**Stage:** 9 onward (`renderer.cpp`, `compile.bat`).

**Symptom:** double-click `App.exe` in the build folder and it fails (then entry 20 hides why). `compile.bat` also hard-codes the author's SDK path (`C:\VulkanSDK\1.2.198.1\...` in Stage 9), which doesn't exist on your PC.

**Bad:**
```cpp
shaders = make_shader_objects(logicalDevice, "shaders/vertex.spv", "shaders/fragment.spv", ...);
```

**Good** (`CMakeLists.txt`, `renderer.cpp`): CMake compiles the shaders with the SDK's `glslc` whenever they change, puts them in `build/shaders/`, and gives the program that absolute folder.
```cpp
shaders = make_shader_objects(logicalDevice,
    SHADER_DIR "/vertex.spv", SHADER_DIR "/fragment.spv", deletionQueue);
```

**Why:** relative paths are relative to the *working directory*, which depends on how the program was started, not on where the .exe is. Same lesson as `RESOURCE_DIR` in MyLearnWebGPU (`MyLearnWebGPU\MyNotes\loadingFromFile.md`).

### 27. Build files tied to one machine

**Stage:** every stage (`StartPoint.vcxproj`).

**Symptom:** the author's projects expect a `dependencies\` folder with `glfw3.lib` and headers copied into each stage; on your PC most stage folders don't have it, so they don't build as downloaded.

**Good:** `CMakeLists.txt` builds GLFW from source and finds the Vulkan SDK through `VULKAN_SDK`. Nothing is copied per stage.

---

# Found by the analysis tools (2026-10-15)

These two were in OUR code as well as the author's: our Stage 10 copied his behavior. The sweep's dynamic tests (scripted resizes, CPU sampling, a profiler) found them; reading the code hadn't.

## Lifetime

### 30. Minimizing the window stops the renderer for good

**Stage:** 10 (ours and the author's; he adds swapchain recreation in Stage 15).

**Symptom:** minimize the window and restore it: the picture is frozen. The log ends with `vk::Queue::presentKHR: ErrorOutOfDateKHR`, the render thread's thread count drops (20 -> 13 on your PC), and only closing the window still works. Resizing it from another program (the sweep's test) does the same.

**Bad** (our Stage 10):
```cpp
// render_thread.cpp: any exception ends the render thread
try {
    Engine engine(window, width, height);
    while (!done->load()) { engine.draw(); }
}
catch (const std::exception& error) { Logger::get_logger().print(error.what()); }
```
and `presentKHR` throws `vk::OutOfDateKHRError` as soon as the window is minimized.

**Good** (`draw.cpp`, `swapchain_rebuild.cpp`):
```cpp
void Engine::draw() {
    if (swapchainOutOfDate) {
        if (!rebuild_swapchain()) {        // still minimized: size 0 x 0
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            return;
        }
        swapchainOutOfDate = false;
    }
    try { draw_frame(); }
    catch (const vk::OutOfDateKHRError&) { swapchainOutOfDate = true; }   // only this error
}
```
`rebuild_swapchain()` waits for the GPU, flushes a second deletion queue that holds only the swapchain's objects (command buffers, image views, the swapchain), and makes them again at the surface's new size. Tested: minimize/restore, three outside resizes and a maximize all keep drawing, with no validation errors and a clean AddressSanitizer run.

**Why:** "out of date" is not a failure, it's Vulkan saying "this swapchain was made for a window that no longer looks like this". The spec: once `VK_ERROR_OUT_OF_DATE_KHR` is returned, presenting to that swapchain will keep failing, so it must be recreated. Treating an expected event as fatal is the same family as entry 26 in reverse: here the error *was* reported, and we stopped when we should have recovered. Source: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#vkQueuePresentKHR

## Performance

### 31. The frame rate has no cap: 5,880 frames per second to show 60

**Stage:** 7/10 (the author's Mailbox choice; later stages pick Immediate).

**Symptom:** 0.78 of a CPU core busy and the GPU awake all the time, for a triangle; on a laptop, fan noise and battery drain. AMD uProf: 62 % of the CPU time inside the NVIDIA driver.

**Bad:**
```cpp
for (vk::PresentModeKHR mode : presentModes) {
    if (mode == vk::PresentModeKHR::eMailbox) { return mode; }
}
return vk::PresentModeKHR::eFifo;
```

**Good** (`swapchain.h` / `swapchain.cpp`): FIFO by default, Mailbox/Immediate on request.
```cpp
inline constexpr vk::PresentModeKHR preferredPresentMode = vk::PresentModeKHR::eFifo;
```
Result: 59.9 frames per second at 0.03 core.

**Why:** measured, step by step, in `stage10b-framePacing.md`. A render loop with nothing to wait for draws as fast as it can; FIFO gives it the monitor to wait for.

---

# Style findings the tools reported (not bugs, but now fixed)

| Tool / check | What it said | What we did |
|---|---|---|
| clang-tidy `cppcoreguidelines-special-member-functions` (32) | classes that `= delete` copying say nothing about moving | added `X(X&&) = delete;` and `operator=(X&&) = delete;` to Engine, GlfwWindow, RenderThread, DeletionQueue, Logger. They were already not movable (a deleted copy hides the moves); now the code says so. |
| clang-tidy `misc-const-correctness` (38) | locals that never change could be `const` | made them `const`. A `const` local tells the reader "this value is final" and lets the compiler catch an accidental change. |
| clang-tidy `readability-make-member-function-const` | `Frame::record_rendering` could be `const` | moved it out of the class (a free function in `frame.cpp`): a "const" method that records GPU commands would read as "changes nothing". |
| clang-tidy `performance-avoid-endl` | `std::endl` flushes | kept on purpose in `Logger::print` (a debug log should reach the screen before a crash), with `// NOLINT(performance-avoid-endl): flush on purpose`. |
| clang-tidy `bugprone-signed-bitwise` | `|` on enum flags | false positive for Vulkan-Hpp flag types; no change. |
| Cppcheck `useInitializationList` | `Frame::imageView` assigned in the constructor body | now set in the initializer list. |
| Cppcheck `useStlAlgorithm` (6) | hand-written search loops | `std::any_of` (instance.cpp, device.cpp), `std::find_if` (swapchain.cpp, instance.cpp), `std::find` (swapchain.cpp). One loop kept (renderer.cpp: building Frames has side effects), with `// cppcheck-suppress useStlAlgorithm` and the reason. |

After the changes: clang-tidy (the sweep's check list plus `-Wdocumentation`) 0 findings, Cppcheck 0, lizard 0 functions over 25 lines, AddressSanitizer clean. Details: `analysis-2026-10-15.md`.
