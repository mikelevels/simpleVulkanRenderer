Stage 8

# Image views

Video: "Vulkan with C++, Stage 8: Image Views", https://www.youtube.com/watch?v=hrEXhisL720
Author's code: `updatedGetIntoGameDev\APIs\vulkan\08 - image views\finished`
WebGPU counterpart: `texture.createView()`, `MyLearnWebGPU\MyNotes\FirstColor.md`.

## What this stage does

Each `Frame` now also creates a **view** of its swapchain image, destroyed at exit (before the swapchain). No new output lines; nothing visible changes.

New files: `image.{h,cpp}`.

## Vulkan ideas

### Image vs image view
A `VkImage` is memory plus a description (size, format, number of mip levels and array layers). Nothing draws into or reads from an image directly; it goes through a `VkImageView`, which picks:
- the **type**: 2D here (could be a cube map, an array...);
- the **format** to read it as (usually the image's own);
- **component mapping** ("swizzle"): e.g. read red as alpha. Identity = no remapping;
- the **subresource range**: which aspect (color, or depth/stencil), which mip levels, which array layers.

Ours: 2D, the swapchain's format, identity, color aspect, mip 0, layer 0. Spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#resources-image-views

### Who owns what
- The swapchain owns its images: destroying the swapchain destroys them; you must NOT destroy them yourself.
- We own the views: `Frame` pushes `destroyImageView` onto the deletion queue. Since views are created after the swapchain, the deletion queue destroys them before it, which is the required order.

## What the video gets wrong

| Claim | Correction |
|---|---|
| The view must be converted to a raw `VkImageView` before the lambda captures it, or it "goes out of scope" | `vk::ImageView` copies just like `VkImageView`. The actual danger is capturing `this` (a temporary Frame) and reading `this->imageView` later. We capture a copy of the handle. |
| Component mapping are "pre-transforms" on the channels | They're swizzles: which source channel each output channel reads. No math is applied. https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#VkComponentMapping |

## C++ refresher

### 1. Constructor member initializer lists
```cpp
Frame::Frame(vk::Image image, ...) : image(image) {   // member `image` = parameter `image`
    imageView = create_image_view(logicalDevice, image, format);
}
```
Members in the list after `:` are initialized directly; assignments in the body happen after default construction. Prefer the list. Inside the list, `image(image)` is unambiguous: the outer name is the member, the inner one the parameter. Source: https://en.cppreference.com/w/cpp/language/constructor · CG C.49

### 2. Value-initializing a struct, then setting fields
```cpp
vk::ImageViewCreateInfo createInfo;          // every field gets Vulkan-Hpp's default
createInfo.viewType = vk::ImageViewType::e2D;
```
Vulkan-Hpp structs have default values for every member (`sType` set correctly, the rest zero or a sensible enum). The C API equivalent is `VkImageViewCreateInfo info{}; info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;`, where forgetting `sType` is a classic bug.

## What we changed from the author's code

| Author | Ours | Why |
|---|---|---|
| converts the view to `VkImageView` to capture it | captures the `vk::ImageView` | it copies fine |
| `.value` from `createImageView` unchecked | exceptions | bug 26 |
| removes the macOS portability code | never had it | Windows only |
