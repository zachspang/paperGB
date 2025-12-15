#include "ppu.h"
#include "cstring"
#include "gb.h"

PPU::PPU(GB* in_gb) :
	gb(in_gb) {
	init_SDL();
	current_mode = VBlank;
	dot_count = 0;
	frame_done = false;
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
	win_line_counter = 0;
	wy_equals_ly = false;
	memset(OAM, 0, sizeof(OAM));
	memset(VRAM, 0, sizeof(VRAM));
	stat_line = false;
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

void PPU::set_renderer_color(int id, uint8_t palette) {
	int color = (palette >> (id * 2)) & 0b11;
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
	lcd_status = (byte & 0b01111000) | (lcd_status & 0b10000111);
}
void PPU::lcd_control_write(uint8_t byte) {
	lcd_control = byte;
}

void PPU::lcd_status_write_bit(uint8_t bit_index, bool bit) {
	if (bit) {
		lcd_status |= (1 << bit_index);
	}
	else {
		lcd_status &= ~(1 << bit_index);
	}
}
void PPU::lcd_control_write_bit(uint8_t bit_index, bool bit) {
	if (bit) {
		lcd_control |= (1 << bit_index);
	}
	else {
		lcd_control &= ~(1 << bit_index);
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
	if (ly == ly_comp && lcd_status_read_bit(6)) {
		lcd_status_write_bit(2, 1);
		check_stat();
	}
	else {
		lcd_status_write_bit(2, 0);
	}
}
void PPU::ly_comp_write(uint8_t value) {
	ly_comp = value;
	if (ly == ly_comp && lcd_status_read_bit(6)) {
		lcd_status_write_bit(2, 1);
		check_stat();
	}
	else {
		lcd_status_write_bit(2, 0);
	}
}

uint8_t PPU::read_OAM(uint16_t addr) {
	//Block if mode 2 or 3 
	if ((current_mode == OAM_scan || current_mode == Draw) && lcd_control_read_bit(7)) {
		return 0xFF;
	}

	addr -= 0xFE00;
	return OAM[addr];
}

void PPU::write_OAM(uint16_t addr, uint8_t byte){
	//Block if mode 2 or 3 
	if ((current_mode == OAM_scan || current_mode == Draw) && lcd_control_read_bit(7)) {
		return;
	}

	addr -= 0xFE00;
	OAM[addr] = byte;
}

uint8_t PPU::read_VRAM(uint16_t addr) {
	//Block if mode 3 
	if (current_mode == Draw && lcd_control_read_bit(7)) {
		return 0xFF;
	}

	addr -= 0x8000;
	return VRAM[addr];
}

void PPU::write_VRAM(uint16_t addr, uint8_t byte) {
	//Block if mode 3 
	if (current_mode == Draw && lcd_control_read_bit(7)) {
		return;
	}

	addr -= 0x8000;
	VRAM[addr] = byte;
}

void PPU::tick() {
	dot_count++;

	//When a mode should be done, its entire duration is executed in one cycle instead of executing each dot individually
	switch (current_mode) {
	case OAM_scan:
		if (dot_count > DOTS_PER_OAM_SCAN) {
			dot_count = 0;
			lcd_status_write_bit(0, 1);
			lcd_status_write_bit(1, 1);
			current_mode = Draw;
		}
		break;
	case Draw:
		if (dot_count > DOTS_PER_DRAW) {
			//Dont need to anything since we are drawing the entire line during HBLANK
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
				win_line_counter = 0;
				wy_equals_ly = false;
				lcd_status_write_bit(0, 1);
				lcd_status_write_bit(1, 0);
				current_mode = VBlank;
				gb->int_vblank();
			}
			else {
				lcd_status_write_bit(0, 0);
				lcd_status_write_bit(1, 1);
				if (win_y == ly) wy_equals_ly = true;
				current_mode = OAM_scan;
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
			if(win_y == ly) wy_equals_ly = true;
			current_mode = OAM_scan;
		}
		break;
	}

	check_stat();
}

//Stat interrupt is only requested when stat_line goes from false to true
void PPU::check_stat() {
	bool starting_stat_line = stat_line;
	if ((lcd_status_read_bit(6) && ly == ly_comp) || 
		(lcd_status_read_bit(5) && current_mode == 2) || 
		(lcd_status_read_bit(4) && current_mode == 1) ||
		(lcd_status_read_bit(3) && current_mode == 0))
	{
		stat_line = true;
	}
	else {
		stat_line = false;
	}
	
	if (starting_stat_line == false && stat_line == true) {
		gb->int_stat();
	}
}

void PPU::draw_line() {
	//Rectangle representing a single pixel
	SDL_Rect pixel = SDL_Rect();
	pixel.w = WINDOW_SCALE_FACTOR;
	pixel.h = WINDOW_SCALE_FACTOR;
	pixel.y = ly * WINDOW_SCALE_FACTOR;

	//Used to check bg color ids to determine object priority
	int bg_color_ids[8 * 32];
	memset(bg_color_ids, 0, sizeof(bg_color_ids));

	//If PPU enabled and bg/window enabled
	if (lcd_control_read_bit(7) && lcd_control_read_bit(0)) {
		//Has the window conditions been true this scanline
		bool win_drawn = false;
		//Internal counter for what tile the window is on
		uint8_t win_tile_x = 0;

		//Which tilemap to use. Used to calculate map_address
		bool tilemap;

		//map_address == 0b11 (1 bit bg_tilemap) (5 bits tile y) (5 bits tile x)
		//This also adjusts for the viewport scrolling
		uint16_t map_address;

		//Index of current tile
		uint16_t tile_index;

		//2 bytes that represent 8 pixels of the current scanline
		uint8_t byte1;
		uint8_t byte2;

		for (int tile_x = 0; tile_x < 32; tile_x++) {
			if (win_drawn || (lcd_control_read_bit(5) && wy_equals_ly && win_x <= 166 && (tile_x * 8) + 7 >= win_x)) {
				//Draw window

				win_drawn = true;
				tilemap = lcd_control_read_bit(6);
				map_address = 0b1100000000000 | (tilemap << 10) | (((win_line_counter / 8) % 32) << 5) | win_tile_x;
				tile_index = VRAM[map_address];
				if (!lcd_control_read_bit(4) && tile_index < 128) {
					tile_index += 256;
				}
				byte1 = VRAM[(tile_index * 0x10) + ((win_line_counter % 8) * 2)];
				byte2 = VRAM[(tile_index * 0x10) + ((win_line_counter % 8) * 2) + 1];
			}
			else {
				//Draw backround

				tilemap = lcd_control_read_bit(3);
				map_address = 0b1100000000000 | (tilemap << 10) | ((((ly + bg_viewport_y) / 8) % 32) << 5) | ((tile_x + (bg_viewport_x / 8)) % 32);
				tile_index = VRAM[map_address];
				if (!lcd_control_read_bit(4) && tile_index < 128) {
					tile_index += 256;
				}
				byte1 = VRAM[(tile_index * 0x10) + (((ly + bg_viewport_y) % 8) * 2)];
				byte2 = VRAM[(tile_index * 0x10) + (((ly + bg_viewport_y) % 8) * 2) + 1];
			}

			//Draw 8 pixels of this tile
			for (int pixel_x = 0; pixel_x < 8; pixel_x++) {
				int x = (tile_x * 8) + pixel_x;
				pixel.x = x * WINDOW_SCALE_FACTOR;

				int color_id = (((byte2 >> (7 - pixel_x)) << 1) & 0b10) | ((byte1 >> (7 - pixel_x)) & 0b1);
				bg_color_ids[x] = color_id;

				set_renderer_color(color_id, bg_palette);
				SDL_RenderFillRect(renderer, &pixel);
			}
			if (win_drawn) win_tile_x++;
		}

		if (win_drawn) win_line_counter++;
	}

	//If PPU and objects are enabled
	if (lcd_control_read_bit(7) && lcd_control_read_bit(1)) {
		//Draw objects

		//Array for each pixel if the scanline. When an object draws an opaque pixel obj_opaque_x_coords[x] = obj_x
		// This is used to determine priority between objects with overlapping opaque pixels
		int obj_opaque_x_coords[255];
		memset(obj_opaque_x_coords, 0, sizeof(obj_opaque_x_coords));
		int obj_this_scanline = 0;
		
		int obj_height = 8;
		if (lcd_control_read_bit(2)) {
			obj_height = 16;
		}

		for (int object_id = 0; object_id < 40; object_id++) {
			uint8_t obj_y = OAM[object_id * 4];
			//The line of the obj tile that will be drawn, 
			// if this is less than 0 or greater than or equal to the object height this object is not on this scanline
			int obj_tile_y = ly - (obj_y - 16);

			//If object is not on this scanline continue
			if (obj_tile_y < 0 || obj_tile_y >= obj_height) {
				continue;
			}

			obj_this_scanline++;
			//If 10 objects have already been drawn this scanline stop drawing objects
			if (obj_this_scanline > 10) {
				break;
			}

			uint8_t obj_x = OAM[(object_id * 4) + 1];
			uint8_t obj_tile_index = OAM[(object_id * 4) + 2];
			uint8_t obj_flags = OAM[(object_id * 4) + 3];

			uint8_t obj_palette;
			//Palette selection
			if ((obj_flags >> 4) & 1) {
				obj_palette = obj_palette1;
			}
			else {
				obj_palette = obj_palette0;
			}

			bool xflip = false;
			//X flip
			if ((obj_flags >> 5) & 1) {
				xflip = true;
			}

			//Y flip
			if ((obj_flags >> 6) & 1) {
				if (obj_height == 16) {
					obj_tile_y = 15 - obj_tile_y;
				}
				else {
					obj_tile_y = 7 - obj_tile_y;
				}
			}

			//In 8x16 mode lsb is ignored, in the lower tile the lsb is always one and in the upper tile the lsb is 0
			if (obj_height == 16) {
				if (obj_tile_y > 7) {
					obj_tile_index |= 0x01;
				}
				else {
					obj_tile_index &= 0xFE;
				}
			}

			bool bg_priority = false;
			//Priority
			if ((obj_flags >> 7) & 1) {
				bg_priority = true;
			}

			uint8_t byte1 = VRAM[(obj_tile_index * 0x10) + ((obj_tile_y % 8) * 2)];
			uint8_t byte2 = VRAM[(obj_tile_index * 0x10) + ((obj_tile_y % 8) * 2) + 1];

			//Draw 8 pixels of object tile
			for (int i = 0; i < 8; i++) {
				int pixel_x = xflip ? (7 - i) : i;
				int x = obj_x - 8 + i;
				pixel.x = x * WINDOW_SCALE_FACTOR;

				if (bg_priority && bg_color_ids[x] > 0) continue;
				int color_id = (((byte2 >> (7 - pixel_x)) << 1) & 0b10) | ((byte1 >> (7 - pixel_x)) & 0b1);
				if (color_id == 0) continue;

				//If an object with higher priority has already drawn an opaque pixel at this coord continue
				if (obj_opaque_x_coords[x] != 0 && obj_x >= obj_opaque_x_coords[x]) continue;
				obj_opaque_x_coords[x] = obj_x;

				set_renderer_color(color_id, obj_palette);
				SDL_RenderFillRect(renderer, &pixel);
			}
		}
	}
}

void PPU::render_frame() {
	SDL_PumpEvents();
	SDL_RenderPresent(renderer);
	SDL_RenderClear(renderer);
	frame_done = true;
}