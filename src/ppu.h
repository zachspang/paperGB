#pragma once
#include "common.h"
#include <SDL.h>
#include "TextureBuffer.h"

class GB;

const int DOTS_PER_OAM_SCAN = 80;
const int DOTS_PER_DRAW = 172;
const int DOTS_PER_HBLANK = 204;
const int DOTS_PER_VBLANK = 4560;
const int WINDOW_SCALE_FACTOR = 5;

class PPU {
public:
	friend class MMU;

	PPU(GB* in_gb, TextureBuffer* emuScreenTexBuffer);

	TextureBuffer* emuScreenTexBuffer;
	bool frame_done;

	//Can only write to bits 6-3
	void lcd_status_write(uint8_t byte);
	void lcd_control_write(uint8_t byte);

	uint8_t read_OAM(uint16_t addr);
	void write_OAM(uint16_t addr, uint8_t byte);

	uint8_t read_VRAM(uint16_t addr);
	void write_VRAM(uint16_t addr, uint8_t byte);

	void ly_comp_write(uint8_t value);

	enum PPUMode {
		HBlank,
		VBlank,
		OAM_scan,
		Draw
	};

	//Execute one cycle;
	void tick();
private:
	//Pointer to GB object to call interrupts
	GB* gb;

	PPUMode current_mode;
	//How many dots have passed this current PPU mode
	int dot_count;
	//Raw pixel buffer. Each pixel is 4 bytes RGBA
	std::vector<uint8_t> pixels;
	SDL_Renderer* renderer;

	void lcd_status_write_bit(uint8_t bit_index, bool bit);
	void lcd_control_write_bit(uint8_t bit_index, bool bit);
	bool lcd_status_read_bit(uint8_t bit_index);
	bool lcd_control_read_bit(uint8_t bit_index);
	void ly_write(uint8_t value);
	//Update bg_viewport x and y by setting them to the values stored in temp_bg_viewport_x/y
	//Should only be called at end of Mode 3 (Draw).
	void update_bg_viewports();

	uint8_t lcd_control;
	uint8_t lcd_status;
	//scy register used in calculations, not updated on write to register, only updated with update_bg_viewports()
	uint8_t bg_viewport_y;
	//temp scy register, NOT used in calculations, this is updated on write to scy
	uint8_t temp_bg_viewport_y;
	//scx register used in calculations, not updated on write to register, only updated with update_bg_viewports()
	uint8_t bg_viewport_x;
	//temp scx register, NOT used in calculations, this is updated on write to scx
	uint8_t temp_bg_viewport_x;
	uint8_t ly;
	uint8_t ly_comp;
	uint8_t bg_palette;
	uint8_t obj_palette0;
	uint8_t obj_palette1;
	uint8_t win_y;
	uint8_t win_x;
	uint8_t win_line_counter;
	bool wy_equals_ly;
	uint8_t VRAM[8 * 1024];
	uint8_t OAM[160];

	void init_SDL();
	void set_pixel_color(int id, uint8_t palette, int x, int y);
	bool stat_line;
	void check_stat();
	void draw_line();
	void render_frame();
};