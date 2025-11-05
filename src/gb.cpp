#include "gb.h"
#include <chrono>

GB::GB(Cartridge in_cart) :
	//cart(in_cart),
	cpu(this),
	ppu(),
	apu(),
	mmu(this),
	timer(),
	input()
{
	t_cycle_count = 0;
}

void GB::tick_other_components() {
	//This is called after an M-Cycle so we need to tick ppu 4 T-cycles
	for (int i = 0; i < 4; i++) {
		t_cycle_count++;
		ppu.tick();
	}
	apu.tick();
	timer.tick();
}

int GB::get_t_cycle_count() {
	return t_cycle_count;
}

void GB::run() {
	//TODO: Change to run 1 frame worth of cycles at full speed then sleep
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