#include <iostream>
#include <SDL.h>
#include "gb.h"
#include "TextureBuffer.h"
#include <thread>
#include "3d.h"

Cartridge* cart = new Cartridge();

// Wrapper function for atexit
void saveAtExit() {
	cart->save();
}

int main(int argc, char* argv[]) {
	//TODO: cleanup SDL in APU, PPU and IO
	atexit(SDL_Quit);

	//TODO: ability to select roms with spaces
	cart->load_rom(argv[1]);
	//handle errors
	atexit(saveAtExit);

	//TextureBuffer used by the emulator and 3d renderer
	TextureBuffer emuScreenTexBuffer;
	//Set size to screen w * h * 4 bytes for RGBA
	emuScreenTexBuffer.pixels.resize(160 * 144 * 4);
	emuScreenTexBuffer.width = 160;
	emuScreenTexBuffer.height = 144;

	// Emulator thread
	std::thread emu([&]() {
		//Allocate Cartridge and GB on heap since they are large
		GB* gameboy = new GB(*cart, &emuScreenTexBuffer);
		gameboy->run();
	});

	// 3d renderer thread
	std::thread renderer([&]() {
		run3d(&emuScreenTexBuffer);
	});

	emu.join();
	renderer.join();
	return 0;
}