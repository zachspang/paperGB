class Cartridge {
//I want Cartridge to handle reads and writes to rom banks and external memory
//MMU.read/write() calls Cartridge for data

public:
	friend class MMU;
	uint8_t* ptr_ROM_bank(uint16_t addr);
	void write_ROM_bank(uint16_t addir, uint8_t byte);

	uint8_t* ptr_RAM(uint16_t addr);
	void write_RAM(uint16_t addir, uint8_t byte);
private:	
	//0000-3FFF
	uint8_t ROMBank00[16 * 1024];

	//4000-7FFF, Switchable ROM bank
	uint8_t ROMBankNN[16 * 1024];

	//A000-BFFF, RAM from cartridge
	uint8_t ExternalRAM[8 * 1024];

	uint8_t boot_ROM_mapping;
};