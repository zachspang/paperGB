#include <cstdint>
#include "gb.h"
class Register {
public:
	virtual uint16_t get_word() = 0;
	virtual void set_word(uint16_t set) = 0;
};

class ByteRegisterPair : public Register {
public:
	uint8_t high;
	uint8_t low;
	uint16_t get_word();
	void set_word(uint16_t set);
};

class WordRegister : public Register {
public:
	uint16_t word;
	uint16_t get_word();
	void set_word(uint16_t set);
};

class CPU {
public:
	friend class MMU;

	CPU(GB& in_gb);

	//Execute one cycle
	void tick();

	void execute_opcode(uint8_t opcode);

	void execute_CB_opcode(uint8_t opcode);

	//Flag enum for accessing the correct bit of F for the corresponding flag
	enum Flag {
		Z = 1 << 7,
		N = 1 << 6,
		H = 1 << 5,
		C = 1 << 4
	};

	void set_flag(Flag flag, bool value);

	bool get_flag(Flag flag);

	//CPU Instructions
	//Info on instructions with not obvious timings 
	// https://gist.github.com/SonoSooS/c0055300670d678b5ae8433e20bea595#fetch-and-stuff

	//Add operand to A + carry flag
	void ADC(uint8_t operand);

	//Add operand to A
	void ADD(uint8_t operand);

	//Add operand to dest
	void ADD(Register& dest, uint16_t operand);

	//Bitwise and A with operand
	void AND(uint8_t operand);

	//Test the bit_idx(0-7) bit of operand and set the zero flag if the bit is not set
	void BIT(uint8_t bit_idx, uint8_t operand);

	//Call addr
	// Ticks 3 M-Cycles
	void CALL(uint16_t addr);

	//Complement carry flag
	void CCF();

	//Compare the value in A with operand, subtract operand from A and set flags then discard result
	void CP(uint8_t operand);

	//Complement A
	void CPL();

	//Decimal Adjust Accumulator
	//https://rgbds.gbdev.io/docs/v0.9.4/gbz80.7#DAA
	void DAA();

	//Decrement dest
	void DEC(uint8_t& dest);

	//Decrement dest
	// Ticks 1 M-Cycles
	void DEC(Register& dest);

	//Disable interrupts byte clearing IME flag
	void DI();
	
	//Enable interrupts by setting IME flag, flag is set after next instruction
	void EI();

	//Enter CPU low-power mode utill an interrupt occurs
	//https://rgbds.gbdev.io/docs/v0.9.4/gbz80.7#HALT
	void HALT();

	//Increment dest
	void INC(uint8_t& dest);

	//Increment Dest
	// Ticks 1 M-Cycles
	void INC(Register& dest);

	//Jump to addr
	// Ticks 1 M-Cycles
	void JP(uint16_t addr);

	//Jump to addr in HL
	void JP();

	//Relative jump to PC + offset(-128,+127)
	void JR(int8_t offset);

	//Copy value of operand to dest
	void LD(uint8_t& dest, uint8_t operand);

	//Copy value of operand to dest
	void LD(Register& dest, uint16_t operand);

	//Copy value of operand to dest then increment HL
	// Ticks 1 M-Cycles
	void LD_HLI(uint8_t& dest, uint8_t operand);

	//Copy value of operand to dest then decrement HL
	// Ticks 1 M-Cycles
	void LD_HLD(uint8_t& dest, uint8_t operand);

	//No operation
	void NOP();

	//Bitwise or A and operand
	void OR(uint8_t operand);

	//Pop from stack and store to dest
	// Ticks 2 M-Cycles
	void POP(Register& dest);

	//Push to stack from reg
	// Tick 3 M-Cycles
	void PUSH(ByteRegisterPair& reg);

	//Set bit bit_idx(0-7) to 0
	void RES(uint8_t bit_idx, uint8_t& dest);

	//Return from subroutine
	// Ticks 3 M-Cycles
	void RET();

	//Return from subroutine and enable interrupts
	void RETI();

	//Rotate bits of operand left through the carry flag
	void RL(uint8_t& operand);

	//Rotate bits of A left through the carry flag
	void RLA();

	//Rotate bits of operand left
	void RLC(uint8_t& operand);

	//Rotate bits of A left
	void RLCA();

	//Rotate bits of operand right through the carry flag
	void RR(uint8_t& operand);

	//Rotate bits of A right through the carry flag
	void RRA();

	//Rotate bits of operand right
	void RRC(uint8_t& operand);

	//Rotate bits of A right
	void RRCA();

	//Call address tgt from RST vector
	// Ticks 3 M-Cycles
	void RST(uint8_t tgt);
	
	//Subtract operand and carry flag from A
	void SBC(uint8_t operand);

	//Set carry flag
	void SCF();

	//Set bit bit_idx(0-7) of dest
	void SET(uint8_t bit_idx, uint8_t& dest);

	//Shift dest left arithmetically
	void SLA(uint8_t& dest);

	//Shift dest right arithmetically
	void SRA(uint8_t& dest);

	//Shift dest right logically
	void SRL(uint8_t& dest);

	//STOP is weird and not used in any licensed games, for now it will just call NOP
	void STOP();

	//Subtract operand from A
	void SUB(uint8_t operand);

	//Swap the upper and lower bits of dest
	void SWAP(uint8_t& dest);

	//Bitwise XOR A and operand
	void XOR(uint8_t operand);

private:
	//GB object, used to access memory
	GB& gb;

	//Registers AF, BC, DE, HL are 16bit registers that can also be two seperate 8bit registers

	//Accumulator register and Flags register, bits 0-3 are empty, 4:carry flag, 5:half carry flag, 6: subtraction flag, 7: zero flag
	ByteRegisterPair AF;

	//Register B and C
	ByteRegisterPair BC;

	//Register D and E
	ByteRegisterPair DE;

	//Register H and L
	ByteRegisterPair HL;

	//Stack pointer
	WordRegister SP;

	//Program counter
	uint16_t PC;

	bool ei_scheduled;

	bool interrupt_master_enable;

	uint8_t interrupt_enable;

	uint8_t interrupt_flag;

	bool halted;
};