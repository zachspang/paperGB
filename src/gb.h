#include <cartridge.h>
#include <cpu.h>
#include <ppu.h>
#include <apu.h>
#include <mmu.h>
#include <timer.h>
#include <input.h>

class GB {
public: 
	//Initialize GB object with a game cartridge and optionally a save state
	GB(Cartridge cart, save_state);

private: 
	Cartridge cart;

	CPU cpu;

	PPU ppu;

	APU apu;

	MMU mmu;

	Timer timer;

	Input input;
};