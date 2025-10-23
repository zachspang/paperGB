#include "gb.h"
#include <chrono>

GB::GB(Cartridge in_cart) :
	//cart(in_cart),
	cpu(*this),
	ppu(),
	apu(),
	mmu(*this),
	timer(),
	input()
{
}

void GB::tick_other_components() {
	//This is called after an M-Cycle so we need to tick ppu 4 T-cycles
	for (int i = 0; i < 4; i++) {
		ppu.tick();
	}
	apu.tick();
	timer.tick();
}

void GB::run() {
	const int MCYCLE_HZ = 1048576;
	auto last_cycle_timestamp = std::chrono::steady_clock::now();
	std::chrono::duration<double> elapsed_time;

	while (true) {
		elapsed_time = (std::chrono::steady_clock::now() - last_cycle_timestamp);
		
		if (elapsed_time.count() > (1.0 / MCYCLE_HZ)) {
			cpu.tick();
			last_cycle_timestamp = std::chrono::steady_clock::now();
		}
	}
}