#include "timer.h"
#include "gb.h"

Timer::Timer(GB* in_gb) :
	gb(in_gb) {
	DIV = 0;
	TIMA = 0;
	TMA = 0;
	TAC = 0;
	div_cycle_counter = 0;
	tima_cycle_counter = 0;
}

void Timer::tick() {
	div_cycle_counter++;
	tima_cycle_counter++;

	if (div_cycle_counter == DIV_INC_RATE) {
		DIV++;
		div_cycle_counter = 0;
	}

	//If TIMA enabled
	if ((TAC >> 2) & 1) {
		int tima_inc_rate = 0;
		switch (TAC & 0b11) {
		case 0:
			tima_inc_rate = 256;
			break;
		case 1:
			tima_inc_rate = 4;
			break;
		case 2:
			tima_inc_rate = 16;
			break;
		case 3:
			tima_inc_rate = 64;
			break;
		}

		if (tima_cycle_counter > tima_inc_rate) {
			TIMA++;
			tima_cycle_counter = 0;

			//If TIMA overflowed
			if (TIMA == 0) {
				TIMA = TMA;
				gb->int_timer();
			}
		}
	}
}