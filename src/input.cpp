#include "input.h"

Input::Input() {
	joypad_input = 0xCF;
}

uint8_t Input::read_joypad() {
	update_buttons();
	return joypad_input;
};

void Input::write_joypad(uint8_t byte) {
	joypad_input = (byte & 0b11110000);
};

void Input::update_buttons() {
	joypad_input &= 0b11110000;

	//If SsBA selected
	if (((joypad_input >> 5) & 1) == 0) {
		start = !keyboard_state[SDL_SCANCODE_GRAVE];
		select = !keyboard_state[SDL_SCANCODE_TAB];
		a = !keyboard_state[SDL_SCANCODE_SPACE];
		b = !keyboard_state[SDL_SCANCODE_R];

		joypad_input |= start << 3 | select << 2 | b << 1 | a;
	}
	//If d-pad selected
	else if (((joypad_input >> 4) & 1) == 0) {
		up = !keyboard_state[SDL_SCANCODE_W];
		down = !keyboard_state[SDL_SCANCODE_S];
		left = !keyboard_state[SDL_SCANCODE_A];
		right = !keyboard_state[SDL_SCANCODE_D];

		joypad_input |= down << 3 | up << 2 | left << 1 | right;
	}
	//If neither selected low nibble reads 0xF
	else {
		joypad_input |= 0xF;
	}
}