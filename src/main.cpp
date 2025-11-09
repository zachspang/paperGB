#include <iostream>
#include <SDL.h>
#include "gb.h"

Cartridge* cart = new Cartridge();

// Wrapper function for atexit
void saveAtExit() {
	cart->save();
}

int main(int argc, char* argv[]) {
	//TODO: cleanup SDL in APU, PPU and IO
	atexit(SDL_Quit);

	//Allocate Cartridge and GB on heap since they are large

	//cart.load_rom(argv[1])
	//handle errors
	atexit(saveAtExit);

	GB* gameboy = new GB(*cart);
	//gameboy.run()

	return 0;
}