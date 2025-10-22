#include "cpu.h"

CPU::CPU(GB& in_gb) :
	gb(in_gb)
{
	A = 0;
	F = 0;
	B = 0;
	C = 0;
	D = 0;
	E = 0;
	H = 0;
	L = 0;
	SP = 0;
	PC = 0;
}