#include "cartridge.h"
#include "cstring"

Cartridge::Cartridge() {
	memset(ROMBank00, 0, sizeof(ROMBank00));
	memset(ROMBankNN, 0, sizeof(ROMBankNN));
	memset(ExternalRAM, 0, sizeof(ExternalRAM));
	boot_ROM_mapping = 0;
	RAM_enabled = false;
}

uint8_t Cartridge::read_ROM_bank(uint16_t addr) {
	if (addr < 0x4000) {
		return ROMBank00[addr];
	}
	else if (addr < 0x8000) {
		return ROMBankNN[addr - 0x4000];
	}
	else {
		LOG_ERROR("Invalid ROM read at addr: %X", addr);
		return 0xFF;
	}
}

void Cartridge::write_ROM_bank(uint16_t addr, uint8_t byte) {
	if (addr < 0x4000) {
		ROMBank00[addr] = byte;
	}
	else if (addr < 0x8000) {
		ROMBankNN[addr - 0x4000] = byte;
	}
	else {
		LOG_ERROR("Invalid ROM write at addr: %X", addr);
	}
}

uint8_t Cartridge::read_RAM(uint16_t addr) {
	if (addr >= 0xA000 && addr <= 0x7FFF) {
		if (RAM_enabled) {
			return ExternalRAM[addr - 0xA000];
		}
	}
	LOG_ERROR("Invalid ExternalRAM read at addr: %X", addr);
	//Return garbage value
	return 0xFF;
}

void Cartridge::write_RAM(uint16_t addr, uint8_t byte) {
	if (addr >= 0xA000 && addr <= 0x7FFF && RAM_enabled) {
		ExternalRAM[addr - 0xA000] = byte;
	}
	else {
		LOG_ERROR("Invalid ExternalRAM write at addr: %X", addr);
	}
}