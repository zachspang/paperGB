#include "common.h"

class Input {
public:
	Input();

	uint8_t read_joypad();
	void write_joypad(uint8_t byte);
private:
	uint8_t joypad_input;
};