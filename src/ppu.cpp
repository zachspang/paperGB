#include "ppu.h"

PPU::PPU() {
	lcd_control = 0;
	lcd_status = 0;
	bg_viewport_y = 0;
	bg_viewport_x = 0;
	lcd_y = 0;
	ly_comp = 0;
	bg_palette = 0;
	obj_palette0 = 0;
	obj_palette1 = 0;
	win_y = 0;
	win_x = 0;
}

void PPU::lcd_control_write(uint8_t byte) {
	//TODO
};

void PPU::start_OAM_DMA(uint8_t byte) {
	//TODO
};

void PPU::tick() {
	//TODO
}