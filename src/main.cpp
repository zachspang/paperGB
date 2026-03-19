#include <iostream>
#include <SDL.h>
#include "gb.h"
#include "SharedBool.h"
#include "TextureBuffer.h"
#include <thread>
#include <chrono>
#include "3d.h"

Cartridge* cart = new Cartridge();

// Wrapper function for atexit
void saveAtExit() {
	cart->save();
}

//Fills texture buffer with default green color
void resetTexBuffer(TextureBuffer* emuScreenTexBuffer) {
	std::lock_guard<std::mutex> lock(emuScreenTexBuffer->mutex);

	for (size_t i = 0; i < emuScreenTexBuffer->pixels.size(); i += 4)
	{
		emuScreenTexBuffer->pixels[i + 0] = 155;
		emuScreenTexBuffer->pixels[i + 1] = 188;
		emuScreenTexBuffer->pixels[i + 2] = 15;
		emuScreenTexBuffer->pixels[i + 3] = 255;
	}
	emuScreenTexBuffer->dirty = true;
}

int main(int argc, char* argv[]) {
	//TODO: cleanup SDL in APU, PPU and IO
	atexit(SDL_Quit);
	atexit(saveAtExit);

	//TextureBuffer used by the emulator and 3d renderer
	TextureBuffer emuScreenTexBuffer;
	//Set size to screen w * h * 4 bytes for RGBA
	emuScreenTexBuffer.pixels.resize(160 * 144 * 4);
	emuScreenTexBuffer.width = 160;
	emuScreenTexBuffer.height = 144;
	resetTexBuffer(&emuScreenTexBuffer);

	//Shared emulator power state
	SharedBool isPowerOn;
	isPowerOn.value = false;


	// Emulator thread
	std::thread emu([&]() {
		while (true) {
			if (isPowerOn.value) {

				//Allocate Cartridge and GB on heap since they are large	
				//TODO: ability to select roms with spaces
				cart->load_rom(argv[1]);
				GB* gameboy = new GB(*cart, &emuScreenTexBuffer, &isPowerOn);
				gameboy->run();

				//Clear out texture buffer
				resetTexBuffer(&emuScreenTexBuffer);
			}
			else {
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
			}
		}
	});

	// 3d renderer thread
	std::thread renderer([&]() {
		run3d(&emuScreenTexBuffer, &isPowerOn);
	});

	emu.join();
	renderer.join();
	return 0;
}