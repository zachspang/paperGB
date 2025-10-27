#include "cpu.h"

//TODO: !!When this function is called 1 M cycle has happened, the individual cases need to take care of other cycles
//If a CPU instruction always takes more cycles than reads/writes move the tick into the function and note the extra cycle(s) 
//in the comments above the instruction
void CPU::execute_opcode(uint8_t opcode) {
	switch (opcode) {
	case 0xCB:
		execute_CB_opcode(gb.mmu.read(PC++));
		
	case 0xE8:
		//ADD SP, e8 4m
		gb.tick_other_components();
		gb.tick_other_components();
		ADD(SP, gb.mmu.read(PC++));
		set_flag(Z, 0);
	}
}

void CPU::execute_CB_opcode(uint8_t opcode) {

}