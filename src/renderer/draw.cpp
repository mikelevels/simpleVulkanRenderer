// draw.cpp: Engine::draw, putting one frame on screen.
//
// Every frame:
//   1. ACQUIRE: ask the swapchain which image we may draw into next;
//   2. SUBMIT: run that image's pre-recorded command buffer on the GPU;
//   3. PRESENT: hand the image back to be shown.
//
// This is the simplest CORRECT version: the CPU waits for the GPU at each
// step (a fence after acquire, waitIdle after submit), so nothing overlaps.
// Stage 12 replaces the waiting with semaphores, which let the GPU order
// the steps by itself. The author's Stage 10 skips step 1 entirely and
// always presents image 0, which the spec forbids (bugsExplained.md, 23).
#include "renderer.h"
#include <cstdint>
#include <limits>

void Engine::draw() {
	std::uint32_t imageIndex = acquire_image();

	vk::SubmitInfo submitInfo;
	submitInfo.setCommandBuffers(frames[imageIndex].commandBuffer);
	graphicsQueue.submit(submitInfo);
	graphicsQueue.waitIdle();   // CPU waits until the GPU has finished drawing

	present_image(imageIndex);
}

std::uint32_t Engine::acquire_image() {
	constexpr std::uint64_t noTimeout = std::numeric_limits<std::uint64_t>::max();
	// The returned index can be "ready" before the presentation engine has
	// actually stopped reading the image; the fence is signaled when it
	// really has. We wait for it on the CPU, then reset it for next frame.
	vk::ResultValue<std::uint32_t> acquired = logicalDevice.acquireNextImageKHR(
		swapchain.chain, noTimeout, nullptr, imageAcquiredFence);
	(void)logicalDevice.waitForFences(imageAcquiredFence, VK_TRUE, noTimeout);
	logicalDevice.resetFences(imageAcquiredFence);
	return acquired.value;
}

void Engine::present_image(std::uint32_t imageIndex) {
	vk::PresentInfoKHR presentInfo;
	presentInfo.setSwapchains(swapchain.chain);
	presentInfo.setImageIndices(imageIndex);
	// eSuboptimalKHR (still works, but the window changed) is returned, not
	// thrown; Stage 15 rebuilds the swapchain when that happens.
	(void)graphicsQueue.presentKHR(presentInfo);
}
