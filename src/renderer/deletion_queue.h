/// @file deletion_queue.h
/// @brief Remembers how to destroy Vulkan objects, and does it newest first.
///
/// Vulkan objects are plain handles (numbers). Nothing destroys them for you:
/// for every vkCreateX there must be a vkDestroyX, and an object must be
/// destroyed BEFORE the objects it was made from (image views before the
/// device, the device before the instance...). Reverse order of creation
/// always satisfies that.
///
/// The author keeps a std::deque<std::function<...>> in the Engine and empties
/// it by hand in ~Engine(). The same idea is popular in the "vkguide.dev"
/// tutorial (https://vkguide.dev/docs/new_chapter_1/vulkan_init_code/).
#pragma once
#include <functional>
#include <utility>
#include <vector>

/// A stack of "destroy" calls, run newest first by flush() or the destructor.
///
/// Because the destructor flushes, a constructor that throws halfway still
/// destroys whatever it had created: the Engine's own destructor never runs
/// in that case, but its members' destructors do (bugsExplained.md, entry 9).
class DeletionQueue {
public:
	DeletionQueue() = default;
	/// Run every remaining deleter.
	~DeletionQueue() { flush(); }

	// Rule of five: the queue owns cleanup work; a copy would run it twice,
	// a move would leave the reader wondering which one runs it. Forbid both.
	DeletionQueue(const DeletionQueue&) = delete;
	DeletionQueue& operator=(const DeletionQueue&) = delete;
	DeletionQueue(DeletionQueue&&) = delete;
	DeletionQueue& operator=(DeletionQueue&&) = delete;

	/// Remember one destroy call.
	/// @param deleter  usually a lambda that captured the handle it destroys,
	///                 e.g. [instance] { instance.destroy(); }
	void push(std::function<void()> deleter) {
		deleters.push_back(std::move(deleter));
	}

	/// Run every deleter, newest first, and forget them.
	void flush() {
		while (!deleters.empty()) {
			deleters.back()();
			deleters.pop_back();
		}
	}

private:
	std::vector<std::function<void()>> deleters;   ///< Oldest first; run from the back.
};
