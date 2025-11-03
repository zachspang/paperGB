#include "common.h"

class APU {
public:
	friend class MMU;

	APU();

	//Execute one cycle
	void tick();

private:
	//FF10-FF14 Sound channel 1
	uint8_t NR10;
	uint8_t NR11;
	uint8_t NR12;
	uint8_t NR13;
	uint8_t NR14;

	//FF16-FF19 Sound channel 2
	uint8_t NR21;
	uint8_t NR22;
	uint8_t NR23;
	uint8_t NR24;

	//FF1A-FF1E Sound channel 3
	uint8_t NR30;
	uint8_t NR31;
	uint8_t NR32;
	uint8_t NR33;
	uint8_t NR34;

	//FF20-FF23 Sound channel 4
	uint8_t NR41;
	uint8_t NR42;
	uint8_t NR43;
	uint8_t NR44;

	//FF24 Master Volume and VIN panning
	uint8_t NR50;

	//FF25 Sound panning
	uint8_t NR51;

	//FF26 Audio master control
	uint8_t NR52;

	//FF30-FF3F
	uint8_t wave[16];
};