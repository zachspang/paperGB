#include "cartridge.h"
#include "cstring"

Cartridge::Cartridge() {
	memset(ROMBank00, 0, sizeof(ROMBank00));
	memset(ROMBankNN, 0, sizeof(ROMBankNN));
	memset(ExternalRAM, 0, sizeof(ExternalRAM));
	boot_ROM_mapping = 0;
}

uint8_t* Cartridge::ptr_ROM_bank(uint16_t addr) {
	if (addr < 0x4000) {
		return &ROMBank00[addr];
	}
	else if (addr < 0x8000) {
		return &ROMBankNN[addr - 0x4000];
	}
	else {
		LOG_ERROR("Invalid ROM read at addr: %X", addr);
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

uint8_t* Cartridge::ptr_RAM(uint16_t addr) {
	if (addr >= 0xA000 && addr <= 0x7FFF) {
		return &ExternalRAM[addr - 0xA000];
	}
	else {
		LOG_ERROR("Invalid ExternalRAM read at addr: %X", addr);
	}
}

void Cartridge::write_RAM(uint16_t addr, uint8_t byte) {
	if (addr >= 0xA000 && addr <= 0x7FFF) {
		ExternalRAM[addr - 0xA000] = byte;
	}
	else {
		LOG_ERROR("Invalid ExternalRAM write at addr: %X", addr);
	}
}