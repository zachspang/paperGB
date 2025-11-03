#include "input.h"

Input::Input() {
	joypad_input = 0;
}

uint8_t* Input::ptr_joypad() {
	return &joypad_input;
};

void Input::write_joypad(uint8_t byte) {
	joypad_input = byte;
};