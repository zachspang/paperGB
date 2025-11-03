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
    case 0x00: break;
    case 0x01: break;
    case 0x02: break;
    case 0x03: break;
    case 0x04: break;
    case 0x05: break;
    case 0x06: break;
    case 0x07: break;
    case 0x08: break;
    case 0x09: break;
    case 0x0A: break;
    case 0x0B: break;
    case 0x0C: break;
    case 0x0D: break;
    case 0x0E: break;
    case 0x0F: break;

    case 0x10: break;
    case 0x11: break;
    case 0x12: break;
    case 0x13: break;
    case 0x14: break;
    case 0x15: break;
    case 0x16: break;
    case 0x17: break;
    case 0x18: break;
    case 0x19: break;
    case 0x1A: break;
    case 0x1B: break;
    case 0x1C: break;
    case 0x1D: break;
    case 0x1E: break;
    case 0x1F: break;

    case 0x20: break;
    case 0x21: break;
    case 0x22: break;
    case 0x23: break;
    case 0x24: break;
    case 0x25: break;
    case 0x26: break;
    case 0x27: break;
    case 0x28: break;
    case 0x29: break;
    case 0x2A: break;
    case 0x2B: break;
    case 0x2C: break;
    case 0x2D: break;
    case 0x2E: break;
    case 0x2F: break;

    case 0x30: break;
    case 0x31: break;
    case 0x32: break;
    case 0x33: break;
    case 0x34: break;
    case 0x35: break;
    case 0x36: break;
    case 0x37: break;
    case 0x38: break;
    case 0x39: break;
    case 0x3A: break;
    case 0x3B: break;
    case 0x3C: break;
    case 0x3D: break;
    case 0x3E: break;
    case 0x3F: break;

    case 0x40: break;
    case 0x41: break;
    case 0x42: break;
    case 0x43: break;
    case 0x44: break;
    case 0x45: break;
    case 0x46: break;
    case 0x47: break;
    case 0x48: break;
    case 0x49: break;
    case 0x4A: break;
    case 0x4B: break;
    case 0x4C: break;
    case 0x4D: break;
    case 0x4E: break;
    case 0x4F: break;

    case 0x50: break;
    case 0x51: break;
    case 0x52: break;
    case 0x53: break;
    case 0x54: break;
    case 0x55: break;
    case 0x56: break;
    case 0x57: break;
    case 0x58: break;
    case 0x59: break;
    case 0x5A: break;
    case 0x5B: break;
    case 0x5C: break;
    case 0x5D: break;
    case 0x5E: break;
    case 0x5F: break;

    case 0x60: break;
    case 0x61: break;
    case 0x62: break;
    case 0x63: break;
    case 0x64: break;
    case 0x65: break;
    case 0x66: break;
    case 0x67: break;
    case 0x68: break;
    case 0x69: break;
    case 0x6A: break;
    case 0x6B: break;
    case 0x6C: break;
    case 0x6D: break;
    case 0x6E: break;
    case 0x6F: break;

    case 0x70: break;
    case 0x71: break;
    case 0x72: break;
    case 0x73: break;
    case 0x74: break;
    case 0x75: break;
    case 0x76: break;
    case 0x77: break;
    case 0x78: break;
    case 0x79: break;
    case 0x7A: break;
    case 0x7B: break;
    case 0x7C: break;
    case 0x7D: break;
    case 0x7E: break;
    case 0x7F: break;

    case 0x80: break;
    case 0x81: break;
    case 0x82: break;
    case 0x83: break;
    case 0x84: break;
    case 0x85: break;
    case 0x86: break;
    case 0x87: break;
    case 0x88: break;
    case 0x89: break;
    case 0x8A: break;
    case 0x8B: break;
    case 0x8C: break;
    case 0x8D: break;
    case 0x8E: break;
    case 0x8F: break;

    case 0x90: break;
    case 0x91: break;
    case 0x92: break;
    case 0x93: break;
    case 0x94: break;
    case 0x95: break;
    case 0x96: break;
    case 0x97: break;
    case 0x98: break;
    case 0x99: break;
    case 0x9A: break;
    case 0x9B: break;
    case 0x9C: break;
    case 0x9D: break;
    case 0x9E: break;
    case 0x9F: break;

    case 0xA0: break;
    case 0xA1: break;
    case 0xA2: break;
    case 0xA3: break;
    case 0xA4: break;
    case 0xA5: break;
    case 0xA6: break;
    case 0xA7: break;
    case 0xA8: break;
    case 0xA9: break;
    case 0xAA: break;
    case 0xAB: break;
    case 0xAC: break;
    case 0xAD: break;
    case 0xAE: break;
    case 0xAF: break;

    case 0xB0: break;
    case 0xB1: break;
    case 0xB2: break;
    case 0xB3: break;
    case 0xB4: break;
    case 0xB5: break;
    case 0xB6: break;
    case 0xB7: break;
    case 0xB8: break;
    case 0xB9: break;
    case 0xBA: break;
    case 0xBB: break;
    case 0xBC: break;
    case 0xBD: break;
    case 0xBE: break;
    case 0xBF: break;

    case 0xC0: break;
    case 0xC1: break;
    case 0xC2: break;
    case 0xC3: break;
    case 0xC4: break;
    case 0xC5: break;
    case 0xC6: break;
    case 0xC7: break;
    case 0xC8: break;
    case 0xC9: break;
    case 0xCA: break;
    case 0xCB: break;
    case 0xCC: break;
    case 0xCD: break;
    case 0xCE: break;
    case 0xCF: break;

    case 0xD0: break;
    case 0xD1: break;
    case 0xD2: break;
    case 0xD3: break;
    case 0xD4: break;
    case 0xD5: break;
    case 0xD6: break;
    case 0xD7: break;
    case 0xD8: break;
    case 0xD9: break;
    case 0xDA: break;
    case 0xDB: break;
    case 0xDC: break;
    case 0xDD: break;
    case 0xDE: break;
    case 0xDF: break;

    case 0xE0: break;
    case 0xE1: break;
    case 0xE2: break;
    case 0xE3: break;
    case 0xE4: break;
    case 0xE5: break;
    case 0xE6: break;
    case 0xE7: break;
    case 0xE8: break;
    case 0xE9: break;
    case 0xEA: break;
    case 0xEB: break;
    case 0xEC: break;
    case 0xED: break;
    case 0xEE: break;
    case 0xEF: break;

    case 0xF0: break;
    case 0xF1: break;
    case 0xF2: break;
    case 0xF3: break;
    case 0xF4: break;
    case 0xF5: break;
    case 0xF6: break;
    case 0xF7: break;
    case 0xF8: break;
    case 0xF9: break;
    case 0xFA: break;
    case 0xFB: break;
    case 0xFC: break;
    case 0xFD: break;
    case 0xFE: break;
    case 0xFF: break;
    default: LOG_WARN("Invalid opcode 0xCB%X", opcode);
    }
}