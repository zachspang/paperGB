#include "apu.h"
#include "cstring"

APU::APU() {
	NR10 = 0;
	NR11 = 0;
	NR12 = 0;
	NR13 = 0;
	NR14 = 0;

	NR21 = 0;
	NR22 = 0;
	NR23 = 0;
	NR24 = 0;

	NR30 = 0;
	NR31 = 0;
	NR32 = 0;
	NR33 = 0;
	NR34 = 0;

	NR41 = 0;
	NR42 = 0;
	NR43 = 0;
	NR44 = 0;

	NR50 = 0;
	NR51 = 0;
	NR52 = 0;

	memset(wave, 0, sizeof(wave));
}

void APU::tick() {
	//TODO
}