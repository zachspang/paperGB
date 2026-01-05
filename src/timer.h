#pragma once
#include "common.h"

class GB;

class Timer {
public:
	friend class MMU;

	Timer(GB* in_gb);

	//Execute one cycle
	void tick();
private:
	//Pointer to GB object to call interrupts
	GB* gb;

	//Div is incremented at 16384Hz which is once every 64 M cycles
	const int DIV_INC_RATE = 64;

	uint8_t DIV;
	uint8_t TIMA;
	uint8_t TMA;
	uint8_t TAC;

	int div_cycle_counter;
	int tima_cycle_counter;
};