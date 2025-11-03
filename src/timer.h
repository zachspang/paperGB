#include "common.h"

class Timer {
public:
	friend class MMU;

	Timer();

	//Execute one cycle
	void tick();
private:
	uint8_t DIV;
	uint8_t TIMA;
	uint8_t TMA;
	uint8_t TAC;
};