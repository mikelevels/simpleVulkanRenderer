Stage 5

# Queue families and the logical device

Video: "Vulkan with C++, Stage 5: Logical Device Creation", https://www.youtube.com/watch?v=Vy2rvsmnE2U
Author's code: `updatedGetIntoGameDev\APIs\vulkan\05 - logical device\finished`
WebGPU counterpart: `adapter.requestDevice()` and `device.getQueue()`, `MyLearnWebGPU\MyNotes\theDevice.md` and `TheCommandQueue.md`.

## What this stage does

Finds a queue family that can do graphics, creates the **logical device** with one queue from it, and gets that queue. New output:
```
Queue family 0: 16 queue(s), { Graphics | Compute | Transfer | SparseBinding }
Queue family 1: 2 queue(s), { Transfer | SparseBinding }
Queue family 2: 8 queue(s), { Compute | Transfer | SparseBinding }
Queue family 3: 1 queue(s), { Transfer | SparseBinding | VideoDecodeKHR }
Queue family 4: 2 queue(s), { Transfer | SparseBinding | VideoEncodeKHR }
GPU has been successfully abstracted!
...
Deleted logical device
```

New file: `logical_device.cpp` (declared in `device.h`).

## Vulkan ideas

### Queues and queue families
The GPU takes work from **queues**. A **queue family** is a group of identical queues with the same abilities. Your TITAN Xp, from the log:
- family 0: everything (graphics, compute, transfer), 16 queues;
- family 1: copies only (a separate copy engine, fast for uploads);
- family 2: compute and copies, 8 queues (work that can run beside graphics);
- families 3 and 4: the hardware video decoder and encoder.

We need one graphics queue; family 0 is the first match. Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#devsandqueues-queues

### The logical device
Your program's own connection to the GPU, with the extensions and features you turned on, and the queues you asked for. Everything made from here on (swapchain, images, shaders, command buffers) belongs to it, and must be destroyed before it. The queues are created with the device and destroyed with it: never destroy a queue yourself. Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#devsandqueues-devices

### Dispatcher level 3
Right after `createDevice`, `VULKAN_HPP_DEFAULT_DISPATCHER.init(device)` fetches device functions with `vkGetDeviceProcAddr`. Device calls then go straight to the driver instead of through the loader's lookup ("trampoline"). The author never does this step (his device calls go through the loader; it works, just slower). Vulkan-Hpp README: https://github.com/KhronosGroup/Vulkan-Hpp#extensions--per-device-function-pointers

### No device layers
`DeviceCreateInfo` still has layer fields, but device layers have been ignored since Vulkan 1.0.13: the instance's layers apply automatically. The author fills them in, and with the logger off sets the count to 1 with a null list (bug 16). Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#extendingvulkan-layers-devicelayerdeprecation

### Queue priority
`DeviceQueueCreateInfo` takes a priority (0.0 to 1.0) per queue. It only ranks queues of the same device against each other; with one queue the value doesn't matter.

## What the video gets wrong

| Claim | Correction |
|---|---|
| Validation layers "need to be done at device creation" too | Deprecated since 1.0.13, see above. |
| The validation layer "will go ahead and enable" portability subset for you | It only reports that the extension is missing. (macOS only; not relevant on Windows.) |
| (correct) Creating the device also creates its queues | Yes. |

## C++ refresher

### 1. Bit flags and `&`
```cpp
if (families[i].queueFlags & vk::QueueFlagBits::eGraphics) { ... }
```
Each ability is one bit of a number. `|` combines bits, `&` tests them. `vk::Flags<>` is Vulkan-Hpp's type-safe wrapper: you can't accidentally mix queue flags with, say, image usage flags.

Backup example with plain integers:
```cpp
constexpr unsigned Read = 1u << 0, Write = 1u << 1, Exec = 1u << 2;   // 001, 010, 100
unsigned permissions = Read | Write;                                  // 011
bool canWrite = (permissions & Write) != 0;                           // true
bool canExec  = (permissions & Exec) != 0;                            // false
```
Source: https://en.cppreference.com/w/cpp/language/operator_arithmetic#Bitwise_logic_operators · https://github.com/KhronosGroup/Vulkan-Hpp#flags

### 2. `static_cast<bool>`
`vk::QueueFlags & vk::QueueFlagBits` gives flags, not a `bool`. Vulkan-Hpp lets you test it in an `if`, but to store it in a `bool` variable we write `static_cast<bool>(...)`, an explicit conversion that's checked by the compiler. Source: https://en.cppreference.com/w/cpp/language/static_cast

### 3. Signed vs unsigned comparison
`for (std::uint32_t i = 0; i < families.size(); ++i)`: `size()` returns `std::size_t` (64-bit unsigned on x64), `i` is 32-bit unsigned; both unsigned, so no warning. The author's `int i < glfwExtensionCount` mixes signed and unsigned: if the signed one were negative it would turn into a huge positive number. MSVC warns at `/W4` (C4018), and our build treats warnings as errors.

### 4. Initializer list for a one-element "array"
```cpp
vk::DeviceCreateInfo deviceInfo({}, queueInfo, {}, extensions);
```
Vulkan-Hpp's `ArrayProxy` parameters accept a single object (`queueInfo`: count 1), a vector (`extensions`), or `{}` (count 0, null pointer), and set both the count and the pointer. That's what removes the count/pointer mismatch of bug 16.

## What we changed from the author's code

| Author | Ours | Why |
|---|---|---|
| device layers, count 1 with null names | no device layers | bug 16 |
| returns `UINT32_MAX` / `nullptr` on failure, caller continues | throws | bug 26 |
| queue family index computed twice | once, stored in the Engine | one source of truth |
| no `init(device)` for the dispatcher | `init(device)` | direct device calls |
| `logger->set_mode(false/true)` around device choice | removed | bug 24 |
