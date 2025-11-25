#include "common.h"
#include <vector>
#include <string>

class Cartridge {
//I want Cartridge to handle reads and writes to rom banks and external memory
//MMU.read/write() calls Cartridge for data

public:
	friend class MMU;

	Cartridge();

	//Load ROM from file
	bool load_rom(char* filename);

	//Save RAM to file
	void save();

	uint8_t read_ROM(uint16_t addr);

	//Writing to ROM doesnt actually write but it changes cartridge registers
	void write_ROM(uint16_t addr, uint8_t byte);

	uint8_t read_RAM(uint16_t addr);
	void write_RAM(uint16_t addr, uint8_t byte);
private:	
	int mbc_num;
	bool banking_mode;

	//Vector holding the entire ROM
	std::vector<uint8_t> ROM;
	int rom_size;
	uint8_t rom_bank_num;
	uint8_t rom_bank_extra_bit;
	//Map address to rom address
	int get_rom_addr(uint16_t addr);

	//Vector for external RAM
	std::vector<uint8_t> RAM;
	int ram_size;
	uint8_t ram_bank_num;
	bool RAM_enabled;
	//Map address to ram address
	int get_ram_addr(uint16_t addr);

	bool has_battery;
	std::string save_path;
};