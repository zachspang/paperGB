#pragma once

#include "common.h"
#include "cartridge.h"
#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "mmu.h"
#include "timer.h"
#include "input.h"
#include "TextureBuffer.h"

class CPU;

class GB {
public: 
	friend class MMU;
	friend class CPU;

	//Initialize GB object with a game cartridge 
	//TODO: and optionally a save state
	GB(Cartridge in_cart, TextureBuffer* emuScreenTexBuffer);

	//Start emulator loop
	void run();

	//Tick the PPU, APU, and Timer
	void tick_other_components();

	int get_t_cycle_count();

	//Request vblank interrupt
	void int_vblank();

	//Request stat interrupt
	void int_stat();
	
	//Request timer interrupt
	void int_timer();

	//Request serial interrupt
	void int_serial();

	//Request joypad interrupt
	void int_joypad();

private: 
	Cartridge cart;

	CPU cpu;

	PPU ppu;

	APU apu;

	MMU mmu;

	Timer timer;

	Input input;

	uint8_t OAM_DMA;

	int t_cycle_count;
};