#include "cpu.h"

uint16_t ByteRegisterPair::get_word() {
	return (high << 8) | low;
}

void ByteRegisterPair::set_word(uint16_t value) {
	low = value & 0xFF;
	high = (value >> 8) & 0xFF;
}

uint16_t WordRegister::get_word() {
	return word;
}

void WordRegister::set_word(uint16_t value) {
	word = value;
}

CPU::CPU(GB& in_gb) :
	gb(in_gb)
{
	AF.high = 0;
	AF.low = 0;
	BC.high = 0;
	BC.low = 0;
	DE.high = 0;
	DE.low = 0;
	HL.high = 0;
	HL.low = 0;
	SP.word = 0;
	PC = 0;
	ei_scheduled = 0;
	interrupt_master_enable = 0;
	interrupt_enable = 0;
	interrupt_flag = 0;
	halted = false;
}

void CPU::tick() {
	if (!halted) {
		//Increment PC, read the byte at PC, then exectute the opcode it refers to
		execute_opcode(gb.mmu.read(PC++));
	}
	else {
		//TODO: handle halted state
	}

	//Handle interrupts
	if (interrupt_master_enable) {
		//TODO: handle interrupts

		//Disable IME
		ei_scheduled = false;
		interrupt_master_enable = false;
	}

	//Set IME which isnt checked again untill after the next instruction
	if (ei_scheduled) {
		ei_scheduled = false;
		interrupt_master_enable = true;
	}
}

void CPU::set_flag(Flag flag, bool value) {
	if (value) {
		AF.low |= flag;
	}
	else {
		AF.low &= ~flag;
	}
}

bool CPU::get_flag(Flag flag) {
	return AF.low & flag;
}

void CPU::ADC(uint8_t operand) {
	uint16_t result = AF.high + operand + get_flag(C);

	set_flag(Z, (result & 0xFF) == 0);
	set_flag(N,0);
	set_flag(H, (((AF.high & 0xF) + (operand & 0xF) + get_flag(C)) & 0x10) == 0x10);
	set_flag(C, result > 0xFF);

	AF.high = result & 0xFF;
}

void CPU::ADD(uint8_t operand) {
	uint16_t result = AF.high + operand;
	
	set_flag(Z, (result & 0xFF) == 0);
	set_flag(N, 0);
	set_flag(H, (((AF.high & 0xF) + (operand & 0xF)) & 0x10) == 0x10);
	set_flag(C, result > 0xFF);

	AF.high = result & 0xFF;
}

void CPU::ADD(Register& dest, uint16_t operand) {
	uint32_t result = dest.get_word() + operand;
	dest.set_word(result & 0xFFFF);

	set_flag(N, 0);
	set_flag(H, (((dest.get_word() & 0xFFF) + (operand & 0xFFF)) & 0x1000) == 0x1000);
	set_flag(C, result > 0xFFFF);
}

void CPU::AND(uint8_t operand) {
	AF.high &= operand;

	set_flag(Z, AF.high == 0);
	set_flag(N, 0);
	set_flag(H, 1);
	set_flag(C, 0);
}

void CPU::BIT(uint8_t bit_idx, uint8_t operand) {
	set_flag(Z, (operand >> bit_idx) ^ 1);
	set_flag(N, 0);
	set_flag(H, 1);
}

void CPU::CALL(uint16_t addr) {
	//Extra cycle
	gb.tick_other_components();
	//Push high byte of addr
	gb.mmu.write(SP.word - 1, ((PC + 1) >> 8) & 0xFF);
	//Push low byte of addr
	gb.mmu.write(SP.word - 2, (PC + 1) & 0xFF);
	//Decrement SP
	SP.word -= 2;
	
	//JP addr
	PC = addr;
}

void CPU::CCF() {
	set_flag(N, 0);
	set_flag(H, 0);
	set_flag(C, get_flag(C));
}

void CPU::CP(uint8_t operand) {
	uint16_t result = AF.high - operand;

	set_flag(Z, result == 0);
	set_flag(N, 1);
	set_flag(H, (((AF.high & 0xF) - (operand & 0xF)) & 0x10) == 0x10);
	set_flag(C, operand > AF.high);
}

void CPU::CPL() {
	AF.high = ~AF.high;
	set_flag(N, 1);
	set_flag(H, 1);
}

void CPU::DAA() {
	uint8_t adjustment = 0;
	if (get_flag(N)) {
		if (get_flag(H)) {
			adjustment += 0x6;
		}

		if (get_flag(C)) {
			adjustment += 0x60;
		}
		AF.high -= adjustment;
	}
	else {
		if (get_flag(H) || ((AF.high & 0xF) > 0x9)) {
			adjustment += 0x6;
		}

		if (get_flag(C) || (AF.high > 0x99)) {
			adjustment += 0x60;
			set_flag(C, 1);
		}
		AF.high += adjustment;
	}
	set_flag(Z, AF.high == 0);
	set_flag(H, 0);
}

void CPU::DEC(uint8_t& dest) {
	dest--;
	set_flag(Z, dest == 0);
	set_flag(N, 1);
	set_flag(H, (((AF.high & 0xF) - (1 & 0xF)) & 0x10) == 0x10);
}

void CPU::DEC(Register& dest) {
	//Extra cycle
	gb.tick_other_components();

	dest.set_word(dest.get_word() - 1);
}

void CPU::DI() {
	interrupt_master_enable = 0;
}

void CPU::EI() {
	ei_scheduled = true;
}

void CPU::HALT() {
	halted = true;
}

void CPU::INC(uint8_t& dest) {
	dest++;
	set_flag(Z, dest == 0);
	set_flag(N, 0);
	set_flag(H, (((dest & 0xF) + (1 & 0xF)) & 0x10) == 0x10);
}

void CPU::INC(Register& dest) {
	//Extra cycle
	gb.tick_other_components();

	dest.set_word(dest.get_word() + 1);
}

void CPU::JP(uint16_t addr) {}

void CPU::JP(uint8_t cond, uint16_t addr) {}

void CPU::JR(int8_t offset) {}

void CPU::JR(uint8_t cond, int8_t offset) {}

void CPU::LD(uint8_t& dest, uint8_t operand) {
	dest = operand;
}

void CPU::LD(Register& dest, uint16_t operand) {
	dest.set_word(operand);
}

void CPU::LD_HLI(uint8_t& dest, uint8_t operand) {}

void CPU::LD_HLD(uint8_t& dest, uint8_t operand) {}

void CPU::NOP() {}

void CPU::OR(uint8_t operand) {}

void CPU::POP(Register& dest) {}

void CPU::PUSH(Register& dest) {}

void CPU::RES(uint8_t bit_idx, uint8_t& dest) {}

void CPU::RET() {}

void CPU::RET(uint8_t cond) {}

void CPU::RETI() {}

void CPU::RL(uint8_t& operand) {}

void CPU::RLA() {}

void CPU::RLC(uint8_t& operand) {}

void CPU::RLCA() {}

void CPU::RR(uint8_t& operand) {}

void CPU::RRA() {}

void CPU::RRC(uint8_t& operand) {}

void CPU::RRCA() {}

void CPU::RST(uint8_t tgt) {}

void CPU::SBC(uint8_t operand) {}

void CPU::SCF() {}

void CPU::SET(uint8_t bit_idx, uint8_t& dest) {}

void CPU::SLA(uint8_t& dest) {}

void CPU::SRA(uint8_t& dest) {}

void CPU::SRL(uint8_t& dest) {}

void CPU::STOP() {}

void CPU::SUB(uint8_t operand) {}

void CPU::SWAP(uint8_t& dest) {}

void CPU::XOR(uint8_t operand) {}