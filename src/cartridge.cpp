#include "cartridge.h"
#include <fstream> 
#include <cmath>

Cartridge::Cartridge() {
	mbc_num = 0;
	RAM_enabled = false;
	has_battery = false;
	rom_size = 0;
	rom_bank_num = 1;
	ram_size = 0;
	ram_bank_num = 0;
	banking_mode = 0;
	rom_bank_extra_bit = 0;
}

bool Cartridge::load_rom(char* filepath) {
	std::ifstream file(filepath, std::ios::binary);

	if (!file) {
		LOG_ERROR("Error opening ROM at: %s", filepath);
		return false;
	}

	file.seekg(0, std::ios::end);
	rom_size = file.tellg();
	file.seekg(0, std::ios::beg);

	ROM = std::vector<uint8_t>(rom_size);

	file.read(reinterpret_cast<char*>(ROM.data()), rom_size);

	if (file.fail()) {
		LOG_ERROR("Failed to read ROM");
		return false;
	}
	file.close();

	//Header checksum
	uint8_t checksum = 0;
	for (uint16_t address = 0x0134; address <= 0x014C; address++) {
		checksum = checksum - ROM[address] - 1;
	}

	if (checksum != ROM[0x014D]) {
		LOG_ERROR("Failed header checksum");
		return false;
	}

	//Get RAM size
	switch (ROM[0x0149]) {
	case 0x00: ram_size = 0; break;
	case 0x01: ram_size = 0; break;
	case 0x02: ram_size = 8 * 1024;  break;
	case 0x03: ram_size = 32 * 1024;  break;
	case 0x04: ram_size = 128 * 1024;  break;
	case 0x05: ram_size = 64 * 1024;  break;
	}

	//Get MBC num and battery
	switch (ROM[0x0147]) {
	case 0x01:
	case 0x02:
		mbc_num = 1;
		has_battery = false;
		break;
	case 0x03:
		mbc_num = 1;
		has_battery = true;
		break;
	case 0x05:
		mbc_num = 2;
		has_battery = false;
		break;
	case 0x06:
		mbc_num = 2;
		has_battery = true;
		break;
	case 0x0F:
	case 0x10:
		mbc_num = 3;
		has_battery = true;
		break;
	case 0x11:
	case 0x12:
		mbc_num = 3;
		has_battery = false;
		break;
	case 0x13:
		mbc_num = 3;
		has_battery = true;
		break;
	case 0x19:
	case 0x1A:
		mbc_num = 5;
		has_battery = false;
		break;
	case 0x1B:
		mbc_num = 5;
		has_battery = true;
		break;
	case 0x1C:
	case 0x1D:
		mbc_num = 5;
		has_battery = false;
		break;
	case 0x1E:
		mbc_num = 5;
		has_battery = true;
		break;
	default:
		mbc_num = 0;
		has_battery = false;
	}
	
	//MBC2 has 512 half-bytes of RAM in the chip
	if (mbc_num == 2) {
		ram_size = 512;
	}

	RAM = std::vector<uint8_t>(ram_size);

	save_path = filepath;
	int dotPos = save_path.rfind('.');
	save_path = save_path.substr(0, dotPos) + ".sav";

	//Load save game or create blank save 
	if (has_battery) {
		std::fstream file(save_path, std::ios::binary);
		if (!file) {
			LOG_ERROR("Failed to open/create save");
		}

		file.read(reinterpret_cast<char*>(RAM.data()), ram_size);

		if (file.fail()) {
			LOG_ERROR("Failed to read save");
		}
		file.close();
	}
}

void Cartridge::save() {
	std::ofstream file(save_path, std::ios::binary);
	if (!file) {
		LOG_ERROR("Failed to open save file");
		return;
	}

	file.write(reinterpret_cast<const char*>(RAM.data()), ram_size);
	file.close();
}

int Cartridge::get_rom_addr(uint16_t addr) {
	switch (mbc_num) {
	case 1:
		if (addr < 0x4000) {
			if (banking_mode == 1) {
				//If > 5 bit bank number is not needed
				if ((rom_size / 1024) / 16 < 32) {
					return addr;
				}
				return (ram_bank_num << 19) + addr;
			}
			else {
				return addr;
			}
		}
		else {
			//If > 5 bit bank number is not needed
			if ((rom_size / 1024) / 16 < 32) {
				return (rom_bank_num << 14) + (addr - 0x4000);
			}
			return (ram_bank_num << 19) + (rom_bank_num << 14) + (addr - 0x4000);
		}
	case 2:
		if (addr < 0x4000) {
			return addr;
		}
		else {
			return (rom_bank_num << 14) + (addr - 0x4000);
		}
	case 3:
		if (addr < 0x4000) {
			return addr;
		}
		else {
			return (rom_bank_num << 14) + (addr - 0x4000);
		}
	case 5:
		if (addr < 0x4000) {
			return addr;
		}
		else {
			return (rom_bank_extra_bit << 22) + (rom_bank_num << 14) + (addr - 0x4000);
		}
	default:
		return addr;
	}
}

int Cartridge::get_ram_addr(uint16_t addr) {
	addr -= 0xA000;
	switch (mbc_num) {
	case 1:
		if (banking_mode == 1) {
			//if only one ram bank
			if ((ram_size / 1024) / 8 == 1) {
				return addr;
			}
			return ram_bank_num << 13 + addr;
		}
		else {
			return addr;
		}
	case 2:
		return addr & 0x1FF;
	case 3:
		return ram_bank_num << 13 + addr;
	case 5:
		return ram_bank_num << 13 + addr;
	default:
		return addr;
	}
}

uint8_t Cartridge::read_ROM(uint16_t addr) {
	if (addr < 0x8000) {
		return ROM[get_rom_addr(addr)];
	}
	else {
		LOG_ERROR("Invalid ROM read at addr: %X", addr);
		return 0xFF;
	}
}

void Cartridge::write_ROM(uint16_t addr, uint8_t byte) {
	switch(mbc_num) {
	case 1:
		if (addr >= 0x0000 && addr <= 0x1FFF) {
			if ((byte & 0xF) == 0xA) {
				RAM_enabled = true;
			}
			else {
				RAM_enabled = false;
				save();
			}
		}
		else if (addr >= 0x2000 && addr <= 0x3FFF) {
			byte = byte & 0x1F;
			if (byte == 0) {
				rom_bank_num = 1;
			}
			else {
				rom_bank_num = byte & ((rom_size / 1024) / 16);
			}
		}
		else if (addr >= 0x4000 && addr <= 0x5FFF) {
			ram_bank_num = byte & 0x03;
		}
		else if (addr >= 0x6000 && addr <= 0x7FFF) {
			banking_mode = byte & 1;
		}
		break;
	case 2:
		if (addr >= 0x0000 && addr <= 0x3FFF) {
			if ((addr >> 8) == 0) {
				if ((byte & 0xF) == 0xA) {
					RAM_enabled = true;
				}
				else {
					RAM_enabled = false;
					save();
				}
			}
			else {
				rom_bank_num = byte & 0xF;
				if (rom_bank_num == 0) rom_bank_num = 1;
			}
		}
		break;
	case 3:
		if (addr >= 0x0000 && addr <= 0x1FFF) {
			if ((byte & 0xF) == 0xA) {
				RAM_enabled = true;
			}
			else if (byte == 0){
				RAM_enabled = false;
				save();
			}
		}
		else if (addr >= 0x2000 && addr <= 0x3FFF) {
			rom_bank_num = byte & 0x7F;
			if (rom_bank_num == 0) rom_bank_num = 1;
		}
		else if (addr >= 0x4000 && addr <= 0x5FFF) {
			if ((byte & 0xF) > 0xC) {
				byte = 0xC;
				LOG_WARN("To large of a value written to RAM Bank Number, defaulting to max acceptable value");
			}
			ram_bank_num = byte & 0xF;
		}
		else if (addr >= 0x6000 && addr <= 0x7FFF) {
			//TODO: Implement RTC registers
			LOG_WARN("Attempted latch unimplemented RTC clock");
		}
		break;
	case 5:
		if (addr >= 0x0000 && addr <= 0x1FFF) {
			if ((byte & 0xF) == 0xA) {
				RAM_enabled = true;
			}
			else if (byte == 0) {
				RAM_enabled = false;
				save();
			}
		}
		else if (addr >= 0x2000 && addr <= 0x2FFF) {
			rom_bank_num = byte;
		}
		else if (addr >= 0x3000 && addr <= 0x3FFF) {
			rom_bank_extra_bit = byte & 1;
		}
		else if (addr >= 0x4000 && addr <= 0x5FFF) {
			ram_bank_num = byte & 0xF;
		}
		break;
	default:
		break;
	}
}

uint8_t Cartridge::read_RAM(uint16_t addr) {
	if (addr >= 0xA000 && addr <= 0x7FFF) {
		if (RAM_enabled) {
			if (mbc_num == 3 && rom_bank_num > 0x7) {
				LOG_WARN("Attempted read from unimplemented RTC registers, read 0x00 instead");
				return 0x00;
			}
			return  RAM[get_ram_addr(addr)];
		}
	}
	LOG_ERROR("Invalid ExternalRAM read at addr: %X", addr);
	//Return garbage value
	return 0xFF;
}

void Cartridge::write_RAM(uint16_t addr, uint8_t byte) {
	if (addr >= 0xA000 && addr <= 0x7FFF && RAM_enabled) {
		RAM[get_ram_addr(addr)] = byte;
	}
	else {
		LOG_ERROR("Invalid ExternalRAM write at addr: %X", addr);
	}
}