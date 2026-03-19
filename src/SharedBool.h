#pragma once
#include <mutex>
// Shared bool value between emulator and 3d renderer
struct SharedBool {
	std::mutex mutex;
	bool value;
	//dirty means new data ready
	bool dirty = false;
};