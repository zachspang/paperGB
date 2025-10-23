#pragma once

#include "cartridge.h"
#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "mmu.h"
#include "timer.h"
#include "input.h"

class GB {
public: 
	friend class MMU;

	//Initialize GB object with a game cartridge 
	//TODO: and optionally a save state
	GB(Cartridge in_cart);

	//Start emulator loop
	void run();

	//Tick the PPU, APU, and Timer
	void tick_other_components();

private: 
	Cartridge cart;

	CPU cpu;

	PPU ppu;

	APU apu;

	MMU mmu;

	Timer timer;

	Input input;

	uint8_t OAM_DMA;
};