// deletion_queue.h: remembers how to destroy every Vulkan object, and does it
// in reverse order of creation.
//
// Vulkan objects are plain handles (numbers). Nothing destroys them for you:
// for every vkCreateX there must be a vkDestroyX, and an object must be
// destroyed BEFORE the objects it was made from (image views before the
// device, the device before the instance...). Reverse order of creation
// always satisfies that.
//
// The author keeps a std::deque<std::function<...>> in the Engine and empties
// it by hand in ~Engine(). The same idea is popular in the "vkguide.dev"
// tutorial (https://vkguide.dev/docs/new_chapter_1/vulkan_init_code/).
// Our version is a small class whose destructor empties the queue: if the
// Engine's constructor throws halfway, the Engine's destructor never runs,
// but its members' destructors DO, so whatever was created gets destroyed
// (bugsExplained.md, entry 9).
#pragma once
#include <functional>
#include <utility>
#include <vector>

class DeletionQueue {
public:
	DeletionQueue() = default;
	~DeletionQueue() { flush(); }

	// Owns cleanup work: copying would run it twice.
	DeletionQueue(const DeletionQueue&) = delete;
	DeletionQueue& operator=(const DeletionQueue&) = delete;

	// Remember one destroy call. `deleter` is usually a lambda that captured
	// the handle it destroys, e.g. [instance] { instance.destroy(); }
	void push(std::function<void()> deleter) {
		deleters.push_back(std::move(deleter));
	}

	// Run every deleter, newest first, and forget them.
	void flush() {
		while (!deleters.empty()) {
			deleters.back()();
			deleters.pop_back();
		}
	}

private:
	std::vector<std::function<void()>> deleters;
};
