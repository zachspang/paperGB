#include "common.h"
#include <SDL.h>

class Input {
public:
	Input();

	uint8_t read_joypad();
	void write_joypad(uint8_t byte);
	
	void update_buttons();

private:
	uint8_t joypad_input;

	const uint8_t* keyboard_state = SDL_GetKeyboardState(nullptr);

	bool start;
	bool select;
	bool b;
	bool a;

	bool up;
	bool down;
	bool left;
	bool right;
};