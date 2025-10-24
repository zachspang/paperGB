#include "gb.h"

class PPU {
public:
	friend class MMU;

	//Can only write to bits 6-3
	void lcd_control_write(uint8_t byte);

	//Start OAM DMA transfer
	void start_OAM_DMA(uint8_t byte);

	//Execute one cycle;
	void tick();
private:
	uint8_t lcd_control;
	uint8_t lcd_status;
	uint8_t bg_viewport_y;
	uint8_t bg_viewport_x;
	uint8_t lcd_y;
	uint8_t ly_comp;
	uint8_t bg_palette;
	uint8_t obj_palette0;
	uint8_t obj_palette1;
	uint8_t win_y;
	uint8_t win_x;
};