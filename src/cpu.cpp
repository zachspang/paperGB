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
	PC.word = 0;
}

void CPU::LD(uint8_t& dest, uint8_t operand) {
	dest = operand;
}

void CPU::LD(Register& dest, uint16_t operand) {
	dest.set_word(operand);
}