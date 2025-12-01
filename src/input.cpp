#include "input.h"

Input::Input() {
	joypad_input = 0xCF;
}

uint8_t Input::read_joypad() {
	return joypad_input;
};

void Input::write_joypad(uint8_t byte) {
	joypad_input = (byte & 0b00110000) | (joypad_input & 0b11001111);
};