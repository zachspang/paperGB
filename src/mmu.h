#pragma once
#include "common.h"

//Forward declaration
class GB;

class MMU {
public:
	MMU(GB* in_gb);

	//Read byte and tick components other than the cpu 1 M-cycle
	uint8_t read(uint16_t addr);

	//Write byte and tick components other than the cpu 1 M-cycle
	void write(uint16_t addr, uint8_t byte);
private:
	GB* gb;

	//8000-9FFF
	uint8_t VRAM[8 * 1024];

	//C000-CFFF
	uint8_t WRAM1[4 * 1024];

	//D000-DFFF
	uint8_t WRAM2[4 * 1024];
	
	//E000-FDFF, Echo RAM, use of this is prohibited

	//FE00-FE9F
	uint8_t OAM[160];
	
	//FF01-FF02, Serial Transfer registers unimplemented
	//uint8_t serial[2];

	//FF80-FFFE
	uint8_t HRAM[127];
};