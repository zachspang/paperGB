#include "cpu.h"
#include "gb.h"

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

CPU::CPU(GB* in_gb) :
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
		//Read the byte at PC, Increment PC, then exectute the opcode it refers to
		execute_opcode(gb->mmu.read(PC++));
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
	//Extra Cycle
	gb->tick_other_components();

	uint32_t result = dest.get_word() + operand;
	
	set_flag(N, 0);
	set_flag(H, (((dest.get_word() & 0xFFF) + (operand & 0xFFF)) & 0x1000) == 0x1000);
	set_flag(C, result > 0xFFFF);

	dest.set_word(result & 0xFFFF);
}

void CPU::ADD_SP_E8(int8_t operand) {
	//Extra Cycle
	gb->tick_other_components();
	//Extra Cycle
	gb->tick_other_components();

	uint32_t result = SP.get_word() + operand;

	set_flag(Z, 0);
	set_flag(N, 0);
	set_flag(H, (((SP.get_word() & 0xFFF) + (operand & 0xFFF)) & 0x1000) == 0x1000);
	set_flag(C, result > 0xFFFF);

	SP.set_word(result & 0xFFFF);
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
	gb->tick_other_components();
	//Push high byte of addr
	gb->mmu.write(SP.word - 1, ((PC + 1) >> 8) & 0xFF);
	//Push low byte of addr
	gb->mmu.write(SP.word - 2, (PC + 1) & 0xFF);
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
	uint8_t result = AF.high - operand;

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
	gb->tick_other_components();

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
	gb->tick_other_components();

	dest.set_word(dest.get_word() + 1);
}

void CPU::JP(uint16_t addr) {
	//Extra cycle
	gb->tick_other_components();

	PC = addr;
}

void CPU::JP_HL() {
	PC = HL.get_word();
}

void CPU::JR(int8_t offset) {
	//Extra Cycles
	gb->tick_other_components();

	PC = PC + 1 + offset;
}

void CPU::LD(uint8_t& dest, uint8_t operand) {
	dest = operand;
}

void CPU::LD(Register& dest, uint16_t operand) {
	dest.set_word(operand);
}

void CPU::LD(uint16_t addr) {
	gb->mmu.write(addr, SP.word & 0xFF);
	gb->mmu.write(addr + 1, SP.word >> 8);
}

void CPU::LD_HLI(uint8_t& dest, uint8_t operand) {
	dest = operand;
	HL.set_word(HL.get_word() + 1);
}

void CPU::LD_HLD(uint8_t& dest, uint8_t operand) {
	dest = operand;
	HL.set_word(HL.get_word() - 1);
}

void CPU::NOP() {}

void CPU::OR(uint8_t operand) {
	AF.high = AF.high | operand;

	set_flag(Z, AF.high == 0);
	set_flag(N, 0);
	set_flag(H, 0);
	set_flag(C, 0);
}

void CPU::POP(Register& dest) {
	uint16_t popped_addr = 0;
	popped_addr =  (gb->mmu.read(SP.word + 1) << 8) | gb->mmu.read(SP.word);
	dest.set_word(popped_addr);
	SP.word += 2;

	//Only POP AF sets flags
	if (&dest == &AF) {
		set_flag(Z, (AF.low >> 7) | 1);
		set_flag(N, (AF.low >> 6) | 1);
		set_flag(H, (AF.low >> 5) | 1);
		set_flag(C, (AF.low >> 4) | 1);
	}
}

void CPU::PUSH(ByteRegisterPair& reg) {
	//Extra cycle
	gb->tick_other_components();
	//Push high byte
	gb->mmu.write(SP.word - 1, reg.high);
	//Push low byte
	gb->mmu.write(SP.word - 2, reg.low);
	//Decrement SP
	SP.word -= 2;
}

void CPU::RES(uint8_t bit_idx, uint8_t& dest) {
	dest &= ~(1 << bit_idx);
}

void CPU::RET() {
	//Extra Cycle
	gb->tick_other_components();

	PC = (gb->mmu.read(SP.word + 1) << 8) | gb->mmu.read(SP.word);
	SP.word += 2;
}

void CPU::RETI() {
	RET();
	interrupt_master_enable = true;
}

void CPU::RL(uint8_t& operand) {
	bool temp_C = get_flag(C);
	set_flag(C, operand >> 7);
	operand = (operand << 1) | temp_C;

	set_flag(Z, operand == 0);
	set_flag(N, 0);
	set_flag(H, 0);
}

void CPU::RLA() {
	bool temp_C = get_flag(C);
	set_flag(C, AF.high >> 7);
	AF.high = (AF.high << 1) | temp_C;

	set_flag(Z, 0);
	set_flag(N, 0);
	set_flag(H, 0);
}

void CPU::RLC(uint8_t& operand) {
	set_flag(C, operand >> 7);
	operand = (operand << 1) | get_flag(C);

	set_flag(Z, operand == 0);
	set_flag(N, 0);
	set_flag(H, 0);
}

void CPU::RLCA() {
	set_flag(C, AF.high >> 7);
	AF.high = (AF.high << 1) | get_flag(C);

	set_flag(Z, 0);
	set_flag(N, 0);
	set_flag(H, 0);
}

void CPU::RR(uint8_t& operand) {
	bool temp_C = get_flag(C);
	set_flag(C, operand & 1);
	operand = (operand >> 1) | (temp_C << 7);

	set_flag(Z, operand == 0);
	set_flag(N, 0);
	set_flag(H, 0);
}

void CPU::RRA() {
	bool temp_C = get_flag(C);
	set_flag(C, AF.high & 1);
	AF.high = (AF.high >> 1) | (temp_C << 7);

	set_flag(Z, 0);
	set_flag(N, 0);
	set_flag(H, 0);
}

void CPU::RRC(uint8_t& operand) {
	set_flag(C, operand & 1);
	operand = (operand >> 1) | (get_flag(C) << 7);

	set_flag(Z, operand == 0);
	set_flag(N, 0);
	set_flag(H, 0);
}

void CPU::RRCA() {
	set_flag(C, AF.high & 1);
	AF.high = (AF.high >> 1) | (get_flag(C) << 7);

	set_flag(Z, 0);
	set_flag(N, 0);
	set_flag(H, 0);
}

void CPU::RST(uint8_t tgt) {
	CALL(tgt);
}

void CPU::SBC(uint8_t operand) {
	operand += get_flag(C);
	uint8_t result = AF.high - operand;

	set_flag(Z, result == 0);
	set_flag(N, 1);
	set_flag(H, (((AF.high & 0xF) - (operand & 0xF)) & 0x10) == 0x10);
	set_flag(C, operand > AF.high);

	AF.high = result;
}

void CPU::SCF() {
	set_flag(N, 0);
	set_flag(H, 0);
	set_flag(C, 1);
}

void CPU::SET(uint8_t bit_idx, uint8_t& dest) {
	dest |= (1 << bit_idx);
}

void CPU::SLA(uint8_t& dest) {
	set_flag(C, dest >> 7);
	dest = dest << 1;

	set_flag(Z, dest == 0);
	set_flag(N, 0);
	set_flag(H, 0);
}

void CPU::SRA(uint8_t& dest) {
	set_flag(C, dest & 1);
	dest = dest >> 1;
	dest |= ((dest << 1) & 0b10000000);

	set_flag(Z, dest == 0);
	set_flag(N, 0);
	set_flag(H, 0);
}

void CPU::SRL(uint8_t& dest) {
	set_flag(C, dest & 1);
	dest = dest >> 1;

	set_flag(Z, dest == 0);
	set_flag(N, 0);
	set_flag(H, 0);
}

void CPU::STOP() {
	NOP();
}

void CPU::SUB(uint8_t operand) {
	uint8_t result = AF.high - operand;

	set_flag(Z, result == 0);
	set_flag(N, 1);
	set_flag(H, (((AF.high & 0xF) - (operand & 0xF)) & 0x10) == 0x10);
	set_flag(C, operand > AF.high);

	AF.high = result;
}

void CPU::SWAP(uint8_t& dest) {
	uint8_t temp = dest << 4;
	dest = dest >> 4;
	dest |= temp;

	set_flag(Z, dest == 0);
	set_flag(N, 0);
	set_flag(H, 0);
	set_flag(C, 0);
}

void CPU::XOR(uint8_t operand) {
	AF.high = AF.high | operand;

	set_flag(Z, AF.high == 0);
	set_flag(N, 0);
	set_flag(H, 0);
	set_flag(C, 0);
}

/*
============================================================================
| Opcodes
============================================================================
*/

uint8_t CPU::n8() {
    return gb->mmu.read(PC++);
}

uint16_t CPU::n16() {
    return (gb->mmu.read(PC++) << 8) | gb->mmu.read(PC++);
}

void CPU::execute_opcode(uint8_t opcode) {
    switch (opcode) {
    case 0xCB: execute_CB_opcode(gb->mmu.read(PC++)); break;

    case 0x00: NOP(); break;
    case 0x01: LD(BC, n16()); break;
    case 0x02: LD(*gb->mmu.ptr(BC.get_word()), AF.high); break;
    case 0x03: INC(BC); break;
    case 0x04: INC(BC.high); break;
    case 0x05: DEC(BC.high); break;
    case 0x06: LD(BC.high, n8()); break;
    case 0x07: RLCA();  break;
    case 0x08: LD(n16()); break;
    case 0x09: ADD(HL, BC.get_word()); break;
    case 0x0A: LD(AF.high, gb->mmu.read(BC.get_word())); break;
    case 0x0B: DEC(BC); break;
    case 0x0C: INC(BC.low); break;
    case 0x0D: DEC(BC.low); break;
    case 0x0E: LD(BC.low, n8()); break;
    case 0x0F: RRCA(); break;

    case 0x10: STOP(); break;
    case 0x11: LD(DE, n16()); break;
    case 0x12: LD(*gb->mmu.ptr(DE.get_word()), AF.high); break;
    case 0x13: INC(DE); break;
    case 0x14: INC(DE.high); break;
    case 0x15: DEC(DE.high); break;
    case 0x16: LD(DE.high, n8()); break;
    case 0x17: RLA();  break;
    case 0x18: JR(n8()); break;
    case 0x19: ADD(HL, DE.get_word()); break;
    case 0x1A: LD(AF.high, gb->mmu.read(DE.get_word())); break;
    case 0x1B: DEC(DE); break;
    case 0x1C: INC(DE.low); break;
    case 0x1D: DEC(DE.low); break;
    case 0x1E: LD(DE.low, n8()); break;
    case 0x1F: RRA(); break;

    case 0x20: if (!get_flag(Z)) JR(n8()); else n8(); break;
    case 0x21: LD(HL, n16()); break;
    case 0x22: LD_HLI(*gb->mmu.ptr(HL.get_word()), AF.high); break;
    case 0x23: INC(HL); break;
    case 0x24: INC(HL.high); break;
    case 0x25: DEC(HL.high); break;
    case 0x26: LD(HL.high, n8()); break;
    case 0x27: DAA();  break;
    case 0x28: if (get_flag(Z)) JR(n8()); else n8(); break;
    case 0x29: ADD(HL, HL.get_word()); break;
    case 0x2A: LD_HLI(AF.high, gb->mmu.read(HL.get_word())); break;
    case 0x2B: DEC(HL); break;
    case 0x2C: INC(HL.low); break;
    case 0x2D: DEC(HL.low); break;
    case 0x2E: LD(HL.low, n8()); break;
    case 0x2F: CPL(); break;

    case 0x30: if (!get_flag(C)) JR(n8()); else n8(); break;
    case 0x31: LD(SP, n16()); break;
    case 0x32: LD_HLD(*gb->mmu.ptr(HL.get_word()), AF.high); break;
    case 0x33: INC(SP); break;
    case 0x34: INC(*gb->mmu.ptr(HL.get_word())); gb->tick_other_components(); break;
    case 0x35: DEC(*gb->mmu.ptr(HL.get_word())); gb->tick_other_components(); break;
    case 0x36: LD(*gb->mmu.ptr(HL.get_word()), n8()); break;
    case 0x37: SCF();  break;
    case 0x38: if (get_flag(C)) JR(n8()); else n8(); break;
    case 0x39: ADD(HL, SP.get_word()); break;
    case 0x3A: LD_HLD(AF.high, gb->mmu.read(HL.get_word())); break;
    case 0x3B: DEC(SP); break;
    case 0x3C: INC(AF.high); break;
    case 0x3D: DEC(AF.high); break;
    case 0x3E: LD(AF.high, n8()); break;
    case 0x3F: CCF(); break;

    case 0x40: LD(BC.high, BC.high); break;
    case 0x41: LD(BC.high, BC.low); break;
    case 0x42: LD(BC.high, DE.high); break;
    case 0x43: LD(BC.high, DE.low); break;
    case 0x44: LD(BC.high, HL.high); break;
    case 0x45: LD(BC.high, HL.low); break;
    case 0x46: LD(BC.high, gb->mmu.read(HL.get_word())); break;
    case 0x47: LD(BC.high, AF.high); break;
    case 0x48: LD(BC.low, BC.high); break;
    case 0x49: LD(BC.low, BC.low); break;
    case 0x4A: LD(BC.low, DE.high); break;
    case 0x4B: LD(BC.low, DE.low); break;
    case 0x4C: LD(BC.low, HL.high); break;
    case 0x4D: LD(BC.low, HL.low); break;
    case 0x4E: LD(BC.low, gb->mmu.read(HL.get_word())); break;
    case 0x4F: LD(BC.low, AF.high); break;

    case 0x50: LD(DE.high, BC.high); break;
    case 0x51: LD(DE.high, BC.low); break;
    case 0x52: LD(DE.high, DE.high); break;
    case 0x53: LD(DE.high, DE.low); break;
    case 0x54: LD(DE.high, HL.high); break;
    case 0x55: LD(DE.high, HL.low); break;
    case 0x56: LD(DE.high, gb->mmu.read(HL.get_word())); break;
    case 0x57: LD(DE.high, AF.high); break;
    case 0x58: LD(DE.low, BC.high); break;
    case 0x59: LD(DE.low, BC.low); break;
    case 0x5A: LD(DE.low, DE.high); break;
    case 0x5B: LD(DE.low, DE.low); break;
    case 0x5C: LD(DE.low, HL.high); break;
    case 0x5D: LD(DE.low, HL.low); break;
    case 0x5E: LD(DE.low, gb->mmu.read(HL.get_word())); break;
    case 0x5F: LD(DE.low, AF.high); break;

    case 0x60: LD(HL.high, BC.high); break;
    case 0x61: LD(HL.high, BC.low); break;
    case 0x62: LD(HL.high, DE.high); break;
    case 0x63: LD(HL.high, DE.low); break;
    case 0x64: LD(HL.high, HL.high); break;
    case 0x65: LD(HL.high, HL.low); break;
    case 0x66: LD(HL.high, gb->mmu.read(HL.get_word())); break;
    case 0x67: LD(HL.high, AF.high); break;
    case 0x68: LD(HL.low, BC.high); break;
    case 0x69: LD(HL.low, BC.low); break;
    case 0x6A: LD(HL.low, DE.high); break;
    case 0x6B: LD(HL.low, DE.low); break;
    case 0x6C: LD(HL.low, HL.high); break;
    case 0x6D: LD(HL.low, HL.low); break;
    case 0x6E: LD(HL.low, gb->mmu.read(HL.get_word())); break;
    case 0x6F: LD(HL.low, AF.high); break;

    case 0x70: LD(*gb->mmu.ptr(HL.get_word()), BC.high); break;
    case 0x71: LD(*gb->mmu.ptr(HL.get_word()), BC.low); break;
    case 0x72: LD(*gb->mmu.ptr(HL.get_word()), DE.high); break;
    case 0x73: LD(*gb->mmu.ptr(HL.get_word()), DE.low); break;
    case 0x74: LD(*gb->mmu.ptr(HL.get_word()), HL.high); break;
    case 0x75: LD(*gb->mmu.ptr(HL.get_word()), HL.low); break;
    case 0x76: HALT(); break;
    case 0x77: LD(*gb->mmu.ptr(HL.get_word()), AF.high); break;
    case 0x78: LD(AF.high, BC.high); break;
    case 0x79: LD(AF.high, BC.low); break;
    case 0x7A: LD(AF.high, DE.high); break;
    case 0x7B: LD(AF.high, DE.low); break;
    case 0x7C: LD(AF.high, HL.high); break;
    case 0x7D: LD(AF.high, HL.low); break;
    case 0x7E: LD(AF.high, gb->mmu.read(HL.get_word())); break;
    case 0x7F: LD(AF.high, AF.high); break;

    case 0x80: ADD(BC.high); break;
    case 0x81: ADD(BC.low); break;
    case 0x82: ADD(DE.high); break;
    case 0x83: ADD(DE.low); break;
    case 0x84: ADD(HL.high); break;
    case 0x85: ADD(HL.low); break;
    case 0x86: ADD(gb->mmu.read(HL.get_word())); break;
    case 0x87: ADD(AF.high); break;
    case 0x88: ADC(BC.high); break;
    case 0x89: ADC(BC.low); break;
    case 0x8A: ADC(DE.high); break;
    case 0x8B: ADC(DE.low); break;
    case 0x8C: ADC(HL.high); break;
    case 0x8D: ADC(HL.low); break;
    case 0x8E: ADC(gb->mmu.read(HL.get_word())); break;
    case 0x8F: ADC(AF.high); break;

    case 0x90: SUB(BC.high); break;
    case 0x91: SUB(BC.low); break;
    case 0x92: SUB(DE.high); break;
    case 0x93: SUB(DE.low); break;
    case 0x94: SUB(HL.high); break;
    case 0x95: SUB(HL.low); break;
    case 0x96: SUB(gb->mmu.read(HL.get_word())); break;
    case 0x97: SUB(AF.high); break;
    case 0x98: SBC(BC.high); break;
    case 0x99: SBC(BC.low); break;
    case 0x9A: SBC(DE.high); break;
    case 0x9B: SBC(DE.low); break;
    case 0x9C: SBC(HL.high); break;
    case 0x9D: SBC(HL.low); break;
    case 0x9E: SBC(gb->mmu.read(HL.get_word())); break;
    case 0x9F: SBC(AF.high); break;

    case 0xA0: AND(BC.high); break;
    case 0xA1: AND(BC.low); break;
    case 0xA2: AND(DE.high); break;
    case 0xA3: AND(DE.low); break;
    case 0xA4: AND(HL.high); break;
    case 0xA5: AND(HL.low); break;
    case 0xA6: AND(gb->mmu.read(HL.get_word())); break;
    case 0xA7: AND(AF.high); break;
    case 0xA8: XOR(BC.high); break;
    case 0xA9: XOR(BC.low); break;
    case 0xAA: XOR(DE.high); break;
    case 0xAB: XOR(DE.low); break;
    case 0xAC: XOR(HL.high); break;
    case 0xAD: XOR(HL.low); break;
    case 0xAE: XOR(gb->mmu.read(HL.get_word())); break;
    case 0xAF: XOR(AF.high); break;

    case 0xB0: OR(BC.high); break;
    case 0xB1: OR(BC.low); break;
    case 0xB2: OR(DE.high); break;
    case 0xB3: OR(DE.low); break;
    case 0xB4: OR(HL.high); break;
    case 0xB5: OR(HL.low); break;
    case 0xB6: OR(gb->mmu.read(HL.get_word())); break;
    case 0xB7: OR(AF.high); break;
    case 0xB8: CP(BC.high); break;
    case 0xB9: CP(BC.low); break;
    case 0xBA: CP(DE.high); break;
    case 0xBB: CP(DE.low); break;
    case 0xBC: CP(HL.high); break;
    case 0xBD: CP(HL.low); break;
    case 0xBE: CP(gb->mmu.read(HL.get_word())); break;
    case 0xBF: CP(AF.high); break;

    case 0xC0: gb->tick_other_components(); if (!get_flag(Z)) RET(); break;
    case 0xC1: POP(BC); break;
    case 0xC2: if (!get_flag(Z)) JP(n16()); else n16(); break;
    case 0xC3: JP(n16());  break;
    case 0xC4: if (!get_flag(Z)) CALL(n16()); else n16(); break;
    case 0xC5: PUSH(BC); break;
    case 0xC6: ADD(n8()); break;
    case 0xC7: RST(0x00);  break;
    case 0xC8: gb->tick_other_components(); if (get_flag(Z)) RET(); break;
    case 0xC9: RET(); break;
    case 0xCA: if (get_flag(Z)) JP(n16()); else n16(); break;
        //case 0xCB: Prefix, case defined at start of switch
    case 0xCC: if (get_flag(Z)) CALL(n16()); else n16(); break;
    case 0xCD: CALL(n16()); break;
    case 0xCE: ADC(n8()); break;
    case 0xCF: RST(0x08); break;

    case 0xD0: gb->tick_other_components(); if (!get_flag(C)) RET(); break;
    case 0xD1: POP(DE); break;
    case 0xD2: if (!get_flag(C)) JP(n16()); else n16(); break;
        //case 0xD3:
    case 0xD4: if (!get_flag(C)) CALL(n16()); else n16(); break;
    case 0xD5: PUSH(DE); break;
    case 0xD6: SUB(n8()); break;
    case 0xD7: RST(0x10);  break;
    case 0xD8: gb->tick_other_components(); if (get_flag(C)) RET(); break;
    case 0xD9: RETI(); break;
    case 0xDA: if (get_flag(C)) JP(n16()); else n16(); break;
        //case 0xDB: 
    case 0xDC: if (get_flag(C)) CALL(n16()); else n16(); break;
        //case 0xDD:
    case 0xDE: SBC(n8()); break;
    case 0xDF: RST(0x18); break;

    case 0xE0: LD(*gb->mmu.ptr(0xFF00 + n8()), AF.high);  break;
    case 0xE1: POP(HL); break;
    case 0xE2: LD(*gb->mmu.ptr(0xFF00 + BC.low), AF.high); break;
        //case 0xE3: 
        //case 0xE4:
    case 0xE5: PUSH(HL); break;
    case 0xE6: AND(n8()); break;
    case 0xE7: RST(0x20); break;
    case 0xE8: ADD_SP_E8(n8()); break;
    case 0xE9: JP_HL();  break;
    case 0xEA: LD(*gb->mmu.ptr(n16()), AF.high); break;
        //case 0xEB:
        //case 0xEC: 
        //case 0xED: 
    case 0xEE: XOR(n8()); break;
    case 0xEF: RST(0x28); break;

    case 0xF0: LD(AF.high, gb->mmu.read(0xFF00 + n8()));  break;
    case 0xF1: POP(AF); break;
    case 0xF2: LD(AF.high, gb->mmu.read(0xFF00 + BC.low)); break;
    case 0xE3: DI(); break;
        //case 0xE4:
    case 0xF5: PUSH(AF); break;
    case 0xF6: OR(n8()); break;
    case 0xF7: RST(0x30); break;
    case 0xF8: gb->tick_other_components(); LD(HL, SP.word + (int8_t)n8()); break;
    case 0xF9: gb->tick_other_components(); LD(SP, HL.get_word());  break;
    case 0xFA: LD(AF.high, gb->mmu.read(n16())); break;
    case 0xEB: EI(); break;
        //case 0xEC: 
        //case 0xED: 
    case 0xFE: CP(n8()); break;
    case 0xFF: RST(0x38); break;

    default: LOG_WARN("Invalid opcode 0x%X", opcode);
    }
}

void CPU::execute_CB_opcode(uint8_t opcode) {
    switch (opcode) {
    case 0x00: RLC(BC.high);  break;
    case 0x01: RLC(BC.low);  break;
    case 0x02: RLC(DE.high);  break;
    case 0x03: RLC(DE.low); break;
    case 0x04: RLC(HL.high);  break;
    case 0x05: RLC(HL.low); break;
    case 0x06: RLC(*gb->mmu.ptr(HL.get_word())); gb->tick_other_components(); break;
    case 0x07: RLC(AF.high); break;

    case 0x08: RRC(BC.high);  break;
    case 0x09: RRC(BC.low);  break; 
    case 0x0A: RRC(DE.high);  break;
    case 0x0B: RRC(DE.low); break; 
    case 0x0C: RRC(HL.high);  break; 
    case 0x0D: RRC(HL.low); break; 
    case 0x0E: RRC(*gb->mmu.ptr(HL.get_word())); gb->tick_other_components(); break;
    case 0x0F: RRC(AF.high); break;

    case 0x10: RL(BC.high);  break;
    case 0x11: RL(BC.low);  break;
    case 0x12: RL(DE.high);  break;
    case 0x13: RL(DE.low); break;
    case 0x14: RL(HL.high);  break;
    case 0x15: RL(HL.low); break;
    case 0x16: RL(*gb->mmu.ptr(HL.get_word())); gb->tick_other_components(); break;
    case 0x17: RL(AF.high); break;

    case 0x18: RR(BC.high);  break;
    case 0x19: RR(BC.low);  break;
    case 0x1A: RR(DE.high);  break;
    case 0x1B: RR(DE.low); break;
    case 0x1C: RR(HL.high);  break;
    case 0x1D: RR(HL.low); break;
    case 0x1E: RR(*gb->mmu.ptr(HL.get_word())); gb->tick_other_components(); break;
    case 0x1F: RR(AF.high); break;

    case 0x20: SLA(BC.high);  break;
    case 0x21: SLA(BC.low);  break;
    case 0x22: SLA(DE.high);  break;
    case 0x23: SLA(DE.low); break;
    case 0x24: SLA(HL.high);  break;
    case 0x25: SLA(HL.low); break;
    case 0x26: SLA(*gb->mmu.ptr(HL.get_word())); gb->tick_other_components(); break;
    case 0x27: SLA(AF.high); break;

    case 0x28: SRA(BC.high);  break;
    case 0x29: SRA(BC.low);  break;
    case 0x2A: SRA(DE.high);  break;
    case 0x2B: SRA(DE.low); break;
    case 0x2C: SRA(HL.high);  break;
    case 0x2D: SRA(HL.low); break;
    case 0x2E: SRA(*gb->mmu.ptr(HL.get_word())); gb->tick_other_components(); break;
    case 0x2F: SRA(AF.high); break;

    case 0x30: SWAP(BC.high);  break;
    case 0x31: SWAP(BC.low);  break;
    case 0x32: SWAP(DE.high);  break;
    case 0x33: SWAP(DE.low); break;
    case 0x34: SWAP(HL.high);  break;
    case 0x35: SWAP(HL.low); break;
    case 0x36: SWAP(*gb->mmu.ptr(HL.get_word())); gb->tick_other_components(); break;
    case 0x37: SWAP(AF.high); break;

    case 0x38: SRL(BC.high);  break;
    case 0x39: SRL(BC.low);  break;
    case 0x3A: SRL(DE.high);  break;
    case 0x3B: SRL(DE.low); break;
    case 0x3C: SRL(HL.high);  break;
    case 0x3D: SRL(HL.low); break;
    case 0x3E: SRL(*gb->mmu.ptr(HL.get_word())); gb->tick_other_components(); break;
    case 0x3F: SRL(AF.high); break;

    case 0x40: BIT(0, BC.high);  break;
    case 0x41: BIT(0, BC.low);  break;
    case 0x42: BIT(0, DE.high);  break;
    case 0x43: BIT(0, DE.low); break;
    case 0x44: BIT(0, HL.high);  break;
    case 0x45: BIT(0, HL.low); break;
    case 0x46: BIT(0, *gb->mmu.ptr(HL.get_word())); break;
    case 0x47: BIT(0, AF.high); break;

    case 0x48: BIT(1, BC.high);  break;
    case 0x49: BIT(1, BC.low);  break;
    case 0x4A: BIT(1, DE.high);  break;
    case 0x4B: BIT(1, DE.low); break;
    case 0x4C: BIT(1, HL.high);  break;
    case 0x4D: BIT(1, HL.low); break;
    case 0x4E: BIT(1, *gb->mmu.ptr(HL.get_word())); break;
    case 0x4F: BIT(1, AF.high); break;

    case 0x50: BIT(2, BC.high);  break;
    case 0x51: BIT(2, BC.low);  break;
    case 0x52: BIT(2, DE.high);  break;
    case 0x53: BIT(2, DE.low); break;
    case 0x54: BIT(2, HL.high);  break;
    case 0x55: BIT(2, HL.low); break;
    case 0x56: BIT(2, *gb->mmu.ptr(HL.get_word())); break;
    case 0x57: BIT(2, AF.high); break;

    case 0x58: BIT(3, BC.high);  break;
    case 0x59: BIT(3, BC.low);  break;
    case 0x5A: BIT(3, DE.high);  break;
    case 0x5B: BIT(3, DE.low); break;
    case 0x5C: BIT(3, HL.high);  break;
    case 0x5D: BIT(3, HL.low); break;
    case 0x5E: BIT(3, *gb->mmu.ptr(HL.get_word())); break;
    case 0x5F: BIT(3, AF.high); break;

    case 0x60: BIT(4, BC.high);  break;
    case 0x61: BIT(4, BC.low);  break;
    case 0x62: BIT(4, DE.high);  break;
    case 0x63: BIT(4, DE.low); break;
    case 0x64: BIT(4, HL.high);  break;
    case 0x65: BIT(4, HL.low); break;
    case 0x66: BIT(4, *gb->mmu.ptr(HL.get_word())); break;
    case 0x67: BIT(4, AF.high); break;

    case 0x68: BIT(5, BC.high);  break;
    case 0x69: BIT(5, BC.low);  break;
    case 0x6A: BIT(5, DE.high);  break;
    case 0x6B: BIT(5, DE.low); break;
    case 0x6C: BIT(5, HL.high);  break;
    case 0x6D: BIT(5, HL.low); break;
    case 0x6E: BIT(5, *gb->mmu.ptr(HL.get_word())); break;
    case 0x6F: BIT(5, AF.high); break;

    case 0x70: BIT(6, BC.high);  break;
    case 0x71: BIT(6, BC.low);  break;
    case 0x72: BIT(6, DE.high);  break;
    case 0x73: BIT(6, DE.low); break;
    case 0x74: BIT(6, HL.high);  break;
    case 0x75: BIT(6, HL.low); break;
    case 0x76: BIT(6, *gb->mmu.ptr(HL.get_word())); break;
    case 0x77: BIT(6, AF.high); break;

    case 0x78: BIT(7, BC.high);  break;
    case 0x79: BIT(7, BC.low);  break;
    case 0x7A: BIT(7, DE.high);  break;
    case 0x7B: BIT(7, DE.low); break;
    case 0x7C: BIT(7, HL.high);  break;
    case 0x7D: BIT(7, HL.low); break;
    case 0x7E: BIT(7, *gb->mmu.ptr(HL.get_word())); break;
    case 0x7F: BIT(7, AF.high); break;

    case 0x80: RES(0, BC.high);  break;
    case 0x81: RES(0, BC.low);  break;
    case 0x82: RES(0, DE.high);  break;
    case 0x83: RES(0, DE.low); break;
    case 0x84: RES(0, HL.high);  break;
    case 0x85: RES(0, HL.low); break;
    case 0x86: RES(0, *gb->mmu.ptr(HL.get_word())); gb->tick_other_components(); break;
    case 0x87: RES(0, AF.high); break;

    case 0x88: RES(1, BC.high);  break;
    case 0x89: RES(1, BC.low);  break;
    case 0x8A: RES(1, DE.high);  break;
    case 0x8B: RES(1, DE.low); break;
    case 0x8C: RES(1, HL.high);  break;
    case 0x8D: RES(1, HL.low); break;
    case 0x8E: RES(1, *gb->mmu.ptr(HL.get_word())); gb->tick_other_components(); break;
    case 0x8F: RES(1, AF.high); break;

    case 0x90: RES(2, BC.high);  break;
    case 0x91: RES(2, BC.low);  break;
    case 0x92: RES(2, DE.high);  break;
    case 0x93: RES(2, DE.low); break;
    case 0x94: RES(2, HL.high);  break;
    case 0x95: RES(2, HL.low); break;
    case 0x96: RES(2, *gb->mmu.ptr(HL.get_word())); gb->tick_other_components(); break;
    case 0x97: RES(2, AF.high); break;

    case 0x98: RES(3, BC.high);  break;
    case 0x99: RES(3, BC.low);  break;
    case 0x9A: RES(3, DE.high);  break;
    case 0x9B: RES(3, DE.low); break;
    case 0x9C: RES(3, HL.high);  break;
    case 0x9D: RES(3, HL.low); break;
    case 0x9E: RES(3, *gb->mmu.ptr(HL.get_word())); gb->tick_other_components(); break;
    case 0x9F: RES(3, AF.high); break;

    case 0xA0: RES(4, BC.high);  break;
    case 0xA1: RES(4, BC.low);  break;
    case 0xA2: RES(4, DE.high);  break;
    case 0xA3: RES(4, DE.low); break;
    case 0xA4: RES(4, HL.high);  break;
    case 0xA5: RES(4, HL.low); break;
    case 0xA6: RES(4, *gb->mmu.ptr(HL.get_word())); gb->tick_other_components(); break;
    case 0xA7: RES(4, AF.high); break;

    case 0xA8: RES(5, BC.high);  break;
    case 0xA9: RES(5, BC.low);  break;
    case 0xAA: RES(5, DE.high);  break;
    case 0xAB: RES(5, DE.low); break;
    case 0xAC: RES(5, HL.high);  break;
    case 0xAD: RES(5, HL.low); break;
    case 0xAE: RES(5, *gb->mmu.ptr(HL.get_word())); gb->tick_other_components(); break;
    case 0xAF: RES(5, AF.high); break;

    case 0xB0: RES(6, BC.high);  break;
    case 0xB1: RES(6, BC.low);  break;
    case 0xB2: RES(6, DE.high);  break;
    case 0xB3: RES(6, DE.low); break;
    case 0xB4: RES(6, HL.high);  break;
    case 0xB5: RES(6, HL.low); break;
    case 0xB6: RES(6, *gb->mmu.ptr(HL.get_word())); gb->tick_other_components(); break;
    case 0xB7: RES(6, AF.high); break;

    case 0xB8: RES(7, BC.high);  break;
    case 0xB9: RES(7, BC.low);  break;
    case 0xBA: RES(7, DE.high);  break;
    case 0xBB: RES(7, DE.low); break;
    case 0xBC: RES(7, HL.high);  break;
    case 0xBD: RES(7, HL.low); break;
    case 0xBE: RES(7, *gb->mmu.ptr(HL.get_word())); gb->tick_other_components(); break;
    case 0xBF: RES(7, AF.high); break;

    case 0xC0: SET(0, BC.high);  break;
    case 0xC1: SET(0, BC.low);  break;
    case 0xC2: SET(0, DE.high);  break;
    case 0xC3: SET(0, DE.low); break;
    case 0xC4: SET(0, HL.high);  break;
    case 0xC5: SET(0, HL.low); break;
    case 0xC6: SET(0, *gb->mmu.ptr(HL.get_word())); gb->tick_other_components(); break;
    case 0xC7: SET(0, AF.high); break;

    case 0xC8: SET(1, BC.high);  break;
    case 0xC9: SET(1, BC.low);  break;
    case 0xCA: SET(1, DE.high);  break;
    case 0xCB: SET(1, DE.low); break;
    case 0xCC: SET(1, HL.high);  break;
    case 0xCD: SET(1, HL.low); break;
    case 0xCE: SET(1, *gb->mmu.ptr(HL.get_word())); gb->tick_other_components(); break;
    case 0xCF: SET(1, AF.high); break;

    case 0xD0: SET(2, BC.high);  break;
    case 0xD1: SET(2, BC.low);  break;
    case 0xD2: SET(2, DE.high);  break;
    case 0xD3: SET(2, DE.low); break;
    case 0xD4: SET(2, HL.high);  break;
    case 0xD5: SET(2, HL.low); break;
    case 0xD6: SET(2, *gb->mmu.ptr(HL.get_word())); gb->tick_other_components(); break;
    case 0xD7: SET(2, AF.high); break;

    case 0xD8: SET(3, BC.high);  break;
    case 0xD9: SET(3, BC.low);  break;
    case 0xDA: SET(3, DE.high);  break;
    case 0xDB: SET(3, DE.low); break;
    case 0xDC: SET(3, HL.high);  break;
    case 0xDD: SET(3, HL.low); break;
    case 0xDE: SET(3, *gb->mmu.ptr(HL.get_word())); gb->tick_other_components(); break;
    case 0xDF: SET(3, AF.high); break;

    case 0xE0: SET(4, BC.high);  break;
    case 0xE1: SET(4, BC.low);  break;
    case 0xE2: SET(4, DE.high);  break;
    case 0xE3: SET(4, DE.low); break;
    case 0xE4: SET(4, HL.high);  break;
    case 0xE5: SET(4, HL.low); break;
    case 0xE6: SET(4, *gb->mmu.ptr(HL.get_word())); gb->tick_other_components(); break;
    case 0xE7: SET(4, AF.high); break;

    case 0xE8: SET(5, BC.high);  break;
    case 0xE9: SET(5, BC.low);  break;
    case 0xEA: SET(5, DE.high);  break;
    case 0xEB: SET(5, DE.low); break;
    case 0xEC: SET(5, HL.high);  break;
    case 0xED: SET(5, HL.low); break;
    case 0xEE: SET(5, *gb->mmu.ptr(HL.get_word())); gb->tick_other_components(); break;
    case 0xEF: SET(5, AF.high); break;

    case 0xF0: SET(6, BC.high);  break;
    case 0xF1: SET(6, BC.low);  break;
    case 0xF2: SET(6, DE.high);  break;
    case 0xF3: SET(6, DE.low); break;
    case 0xF4: SET(6, HL.high);  break;
    case 0xF5: SET(6, HL.low); break;
    case 0xF6: SET(6, *gb->mmu.ptr(HL.get_word())); gb->tick_other_components(); break;
    case 0xF7: SET(6, AF.high); break;

    case 0xF8: SET(7, BC.high);  break;
    case 0xF9: SET(7, BC.low);  break;
    case 0xFA: SET(7, DE.high);  break;
    case 0xFB: SET(7, DE.low); break;
    case 0xFC: SET(7, HL.high);  break;
    case 0xFD: SET(7, HL.low); break;
    case 0xFE: SET(7, *gb->mmu.ptr(HL.get_word())); gb->tick_other_components(); break;
    case 0xFF: SET(7, AF.high); break;

    default: LOG_WARN("Invalid opcode 0xCB%X", opcode);
    }
}