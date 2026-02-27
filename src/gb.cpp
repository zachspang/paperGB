#include "gb.h"
#include <chrono>
#include <thread>

GB::GB(Cartridge in_cart, TextureBuffer* emuScreenTexBuffer) :
	cart(in_cart),
	cpu(this),
	ppu(this, emuScreenTexBuffer),
	apu(),
	mmu(this),
	timer(this),
	input()
{
	OAM_DMA = 0xFF;
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
	const double TARGET_FPS = 59.737;
	auto target_ns = std::chrono::nanoseconds(static_cast<long long>(1e9 / TARGET_FPS));
	//Sleep happen for duration - sleep_buffer to give time for OS sleep inconsistency
	const auto sleep_buffer = std::chrono::milliseconds(2); 

	auto last_frame_timestamp = std::chrono::steady_clock::now();
	auto next_frame_time = last_frame_timestamp + target_ns;

	double total_frametime = 0;
	int frame_count = 0;
	std::chrono::duration<double> frametime;

	while (true) {
		//Run CPU until PPU finishes a frame
		while (!ppu.frame_done) {
			cpu.tick();
		}

		//Frame completed
		auto now = std::chrono::steady_clock::now();
		frametime = now - last_frame_timestamp;
		total_frametime += frametime.count();

		frame_count++;
		if (frame_count % 60 == 0) {
			LOG("AVG FRAMETIME: %f", total_frametime / 60);
			total_frametime = 0;
		}

		//Save SRAM every minute (60 fps * 60 sec = 3600 frames)
		if (frame_count == 3600) {
			frame_count = 0;
			cart.save();
		}

		last_frame_timestamp = now;
		ppu.frame_done = false;

		//Schedule next frame
		next_frame_time += target_ns;
		now = std::chrono::steady_clock::now();
		if (now + sleep_buffer < next_frame_time) {
			//Sleep until shortly before the deadline
			std::this_thread::sleep_until(next_frame_time - sleep_buffer);
		}

		//Busy-wait after waking until enough time has passed
		while (std::chrono::steady_clock::now() < next_frame_time) {
			// spin
		}

		now = std::chrono::steady_clock::now();

		//If 5 frames behind schedule next frame to now
		// Without this in a scenario where there is a large stutter emulation would speed up
		// as fast as possible to catch back up to the schedule. This lets the stutter happen
		// then emulation will stay normal speed after.
		if (now > next_frame_time + (target_ns * 5)) {
			next_frame_time = now;
		}
	}
}


void GB::int_vblank() {
	cpu.interrupt_flag |= 0b1;
}
void GB::int_stat() {
	cpu.interrupt_flag |= 0b10;
}
void GB::int_timer() {
	cpu.interrupt_flag |= 0b100;
}
void GB::int_serial() {
	cpu.interrupt_flag |= 0b1000;
}
void GB::GB::int_joypad() {
	cpu.interrupt_flag |= 0b10000;
}