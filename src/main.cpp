#include <iostream>
#include <SDL.h>

int main(int argc, char* argv[]) {
	//TODO: cleanup SDL in APU, PPU and IO
	atexit(SDL_Quit);

	//cart = cartridge(argv[1]);
	//handle errors

	//save_state = load_save_state(argv[2])
	//handle errors

	//gameboy = GB(cart, save_state);
	//gameboy.run()

	return 0;
}