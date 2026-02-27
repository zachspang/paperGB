#pragma once
#include <mutex>
#include <vector>
// Shared texture buffer that is written to by the emulator and read by the 3d renderer
struct TextureBuffer {
	std::mutex mutex;
	std::vector<uint8_t> pixels;
	//dirty means new data ready
	bool dirty = false;
	int width, height;
};