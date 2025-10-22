#include <cstdint>
#include "gb.h"

class CPU {
public:
	CPU(GB& in_gb);

	void execute_opcode(uint8_t opcode);

	void execute_CB_opcode(uint8_t opcode);

private:
	//GB object, used to access memory
	GB& gb;

	//Registers AF, BC, DE, HL are 16bit registers that can also be two seperate 8bit registers

	//Accumulator register
	uint8_t A;
	//Flags register, bits 0-3 are empty, 4:carry flag, 5:half carry flag, 6: subtraction flag, 7: zero flag
	uint8_t F;

	//Register B
	uint8_t B;
	//Register C
	uint8_t C;

	//Register D
	uint8_t D;
	//Register E
	uint8_t E;

	//Register H
	uint8_t H;
	//Register L
	uint8_t L;

	//Stack pointer
	uint16_t SP;

	//Program counter
	uint16_t PC;
	
};