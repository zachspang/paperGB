#include "ppu.h"
#include "cstring"
#include "gb.h"

PPU::PPU(GB* in_gb) :
	gb(in_gb) {
	init_SDL();
	current_mode = OAM_scan;
	dot_count = 0;
	frame_done - false;
	lcd_control = 0x91;
	lcd_status = 0x85;
	bg_viewport_y = 0;
	bg_viewport_x = 0;
	ly = 0;
	ly_comp = 0;
	bg_palette = 0xFC;
	obj_palette0 = 0;
	obj_palette1 = 0;
	win_y = 0;
	win_x = 0;
	memset(OAM, 0, sizeof(OAM));
	memset(VRAM, 0, sizeof(VRAM));
}

void PPU::init_SDL() {
	//Initialize SDL Video
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		LOG_ERROR("Unable to initialize SDL: %s", SDL_GetError());
		return;
	}

	SDL_Window* window = SDL_CreateWindow(
		"paperGB",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		160 * WINDOW_SCALE_FACTOR,
		144 * WINDOW_SCALE_FACTOR,
		0
	);

	renderer = SDL_CreateRenderer(window, -1, 0);
}

void PPU::set_renderer_color(int color) {
	switch (color) {
	case 0:
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		break;
	case 1:
		SDL_SetRenderDrawColor(renderer, 170, 170, 170, 255);
		break;
	case 2:
		SDL_SetRenderDrawColor(renderer, 85, 85, 85, 255);
		break;
	case 3:
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		break;
	}
}

void PPU::lcd_status_write(uint8_t byte) {
	lcd_status = byte & 0b01111000;
}
void PPU::lcd_control_write(uint8_t byte) {
	lcd_control = byte;
}

void PPU::lcd_status_write_bit(uint8_t bit_index, bool bit) {
	if (bit) {
		lcd_status |= (1 << bit_index);
	}
	else {
		lcd_status &= (0 << bit_index);
	}
}
void PPU::lcd_control_write_bit(uint8_t bit_index, bool bit) {
	if (bit) {
		lcd_control |= (1 << bit_index);
	}
	else {
		lcd_control &= (0 << bit_index);
	}
}

bool PPU::lcd_status_read_bit(uint8_t bit_index) {
	return (lcd_status >> bit_index) & 1;
}
bool PPU::lcd_control_read_bit(uint8_t bit_index) {
	return (lcd_control >> bit_index) & 1;
}

void PPU::ly_write(uint8_t value) {
	ly = value;
	if (ly == ly_comp) {
		gb->int_stat();
		lcd_status_write_bit(2, 1);
	}
	else {
		lcd_status_write_bit(2, 0);
	}
}
void PPU::ly_comp_write(uint8_t value) {
	ly_comp = value;
	if (ly == ly_comp) {
		gb->int_stat();
		lcd_status_write_bit(2, 1);
	}
	else {
		lcd_status_write_bit(2, 0);
	}
}

uint8_t PPU::read_OAM(uint16_t addr) {
	//Block if mode 2 or 3 
	if (current_mode == OAM_scan || current_mode == Draw) {
		return 0xFF;
	}

	addr -= 0xFE00;
	return OAM[addr];
}

void PPU::write_OAM(uint16_t addr, uint8_t byte){
	//Block if mode 2 or 3 
	if (current_mode == OAM_scan || current_mode == Draw) {
		return;
	}

	addr -= 0xFE00;
	OAM[addr] = byte;
}

uint8_t PPU::read_VRAM(uint16_t addr) {
	//Block if mode 3 
	if (current_mode == Draw) {
		return 0xFF;
	}

	addr -= 0x8000;
	return VRAM[addr];
}

void PPU::write_VRAM(uint16_t addr, uint8_t byte) {
	//Block if mode 3 
	//TODO: This blocks when it shouldnt causing broken tiles in vram
	//if (current_mode == Draw) {
	//	return;
	//}

	addr -= 0x8000;
	VRAM[addr] = byte;
}

void PPU::tick() {
	dot_count++;

	//When a mode should be done, its entire duration is executed in one cycle instead of executing each dot individually
	switch (current_mode) {
	case OAM_scan:
		frame_done = false;
		if (dot_count > DOTS_PER_OAM_SCAN) {
			SDL_RenderClear(renderer);
			dot_count = 0;
			lcd_status_write_bit(0, 1);
			lcd_status_write_bit(1, 1);
			current_mode = Draw;
		}
		break;
	case Draw:
		if (dot_count > DOTS_PER_DRAW) {
			//Dont need to anything since we are drawing the entire line during HBLANK
			//TODO: might need to check if interrupts need to be called
			dot_count = 0;
			lcd_status_write_bit(0, 0);
			lcd_status_write_bit(1, 0);
			current_mode = HBlank;
		}
		break;
	case HBlank:
		if (dot_count > DOTS_PER_HBLANK) {
			draw_line();
			ly_write(ly + 1);
			dot_count = 0;
			
			if (ly == 144) {
				lcd_status_write_bit(0, 1);
				lcd_status_write_bit(1, 0);
				current_mode = VBlank;
			}
			else {
				lcd_status_write_bit(0, 1);
				lcd_status_write_bit(1, 1);
				current_mode = Draw;
			}
		}
		break;
	case VBlank:
		//During vblank 10 invisible lines are drawn but this might not matter
		if (dot_count % 456 == 0) {
			ly_write(ly + 1);
		}
		if (dot_count > DOTS_PER_VBLANK) {
			render_frame();
			ly_write(0);

			dot_count = 0;
			lcd_status_write_bit(0, 0);
			lcd_status_write_bit(1, 1);
			current_mode = OAM_scan;
			frame_done = true;
		}
		break;
	}
}

void PPU::draw_line() {
	//Rectangle representing a single pixel
	SDL_Rect pixel = SDL_Rect();
	pixel.w = WINDOW_SCALE_FACTOR;
	pixel.h = WINDOW_SCALE_FACTOR;
	pixel.y = ly * WINDOW_SCALE_FACTOR;

	for (int tile = 0; tile < 32; tile++) {
		//Get pixel color for backround, window and object

		//Backround and window enabled
		if (lcd_control_read_bit(7)) {
			//Draw backround
			bool bg_tilemap = lcd_control_read_bit(3);
			uint16_t map_address = 0b1100000000000 | (bg_tilemap << 10) | ((ly / 8) << 5) | tile;

			//Index of current tile
			uint8_t tile_index = VRAM[map_address];

			//Check addressing mode
			if (!lcd_control_read_bit(4) && tile_index < 128) {
				tile_index += 256;
			}

			//Get 2 bytes that represent 8 pixels of the current scanline
			uint8_t byte1 = VRAM[(tile_index * 0x10) + ((ly % 8) * 2)];
			uint8_t byte2 = VRAM[(tile_index * 0x10) + ((ly % 8) * 2) + 1];

			for (int tile_x = 0; tile_x < 8; tile_x++) {
				int x = (tile * 8) + tile_x;
				pixel.x = x * WINDOW_SCALE_FACTOR;
				int color = (((byte2 >> (7 - tile_x)) << 1) & 0b10) | ((byte1 >> (7 - tile_x)) & 0b1);
				set_renderer_color(color);
				SDL_RenderFillRect(renderer, &pixel);
			}
			
			//Window enabled
			if (lcd_control_read_bit(5)) {

			}
		}
		
		//TODO: draw objects
	}

}

void PPU::render_frame() {
	SDL_PumpEvents();
	SDL_RenderPresent(renderer);
}