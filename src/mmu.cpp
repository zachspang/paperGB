#include "mmu.h"
#include "cstring"

MMU::MMU(GB& in_gb) :
	gb(in_gb)
{
	memset(VRAM, 0, sizeof(VRAM));
	memset(WRAM1, 0, sizeof(WRAM1));
	memset(WRAM2, 0, sizeof(WRAM2));
	memset(OAM, 0, sizeof(OAM));
	memset(HRAM, 0, sizeof(HRAM));
}

uint8_t MMU::read(uint16_t addr) {
	gb.tick_other_components();

	if (addr >= 0x0000 and addr <= 0x7FFF) {
		return gb.cart.read_ROM_bank(addr);
	}
	else if (addr >= 0x8000 and addr <= 0x9FFF) {
		return VRAM[addr - 0x8000];
	}
	else if (addr >= 0xA000 and addr <= 0xBFFF) {
		return gb.cart.read_RAM(addr - 0xA000);
	}
	else if (addr >= 0xC000 and addr <= 0xCFFF) {
		return WRAM1[addr - 0xC000];
	}
	else if (addr >= 0xD000 and addr <= 0xDFFF) {
		return WRAM2[addr - 0xD000];
	}
	else if (addr >= 0xE000 and addr <= 0xFDFF) {
		return 0xFF;
	}
	else if (addr >= 0xFE00 and addr <= 0xFD9F) {
		return OAM[addr - 0xFE00];
	}
	else if (addr >= 0xFEA0 and addr <= 0xFEFF) {
		//Should return 0xFF during OAM block
		return 0x00;
	}
	else if (addr == 0xFF00) {
		return gb.input.read_joypad();
	}
	else if (addr == 0xFF04) {
		return gb.timer.DIV;
	}
	else if (addr == 0xFF05) {
		return gb.timer.TIMA;
	}
	else if (addr == 0xFF06) {
		return gb.timer.TMA;
	}
	else if (addr == 0xFF07) {
		return gb.timer.TAC;
	}
	else if (addr == 0xFF0F) {
		return gb.cpu.interrupt_flag;
	}
	else if (addr == 0xFF10) {
		return gb.apu.NR10;
	}
	else if (addr == 0xFF11) {
		return gb.apu.NR11;
	}
	else if (addr == 0xFF12) {
		return gb.apu.NR12;
	}
	else if (addr == 0xFF13) {
		return gb.apu.NR13;
	}
	else if (addr == 0xFF14) {
		return gb.apu.NR14;
	}
	else if (addr == 0xFF16) {
		return gb.apu.NR21;
	}
	else if (addr == 0xFF17) {
		return gb.apu.NR22;
	}
	else if (addr == 0xFF18) {
		return gb.apu.NR23;
	}
	else if (addr == 0xFF19) {
		return gb.apu.NR24;
	}
	else if (addr == 0xFF1A) {
		return gb.apu.NR30;
	}
	else if (addr == 0xFF1B) {
		return gb.apu.NR31;
	}
	else if (addr == 0xFF1C) {
		return gb.apu.NR32;
	}
	else if (addr == 0xFF1D) {
		return gb.apu.NR33;
	}
	else if (addr == 0xFF1E) {
		return gb.apu.NR34;
	}
	else if (addr == 0xFF20) {
		return gb.apu.NR41;
	}
	else if (addr == 0xFF21) {
		return gb.apu.NR42;
	}
	else if (addr == 0xFF22) {
		return gb.apu.NR43;
	}
	else if (addr == 0xFF23) {
		return gb.apu.NR44;
	}
	else if (addr == 0xFF24) {
		return gb.apu.NR50;
	}
	else if (addr == 0xFF25) {
		return gb.apu.NR51;
	}
	else if (addr == 0xFF26) {
		return gb.apu.NR52;
	}
	else if (addr >= 0xFF30 and addr <= 0xFF3F) {
		return gb.apu.wave[addr - 0xFF30];
	}
	else if (addr == 0xFF40) {
		return gb.ppu.lcd_control;
	}
	else if (addr == 0xFF41) {
		return gb.ppu.lcd_status;
	}
	else if (addr == 0xFF42) {
		return gb.ppu.bg_viewport_y;
	}
	else if (addr == 0xFF43) {
		return gb.ppu.bg_viewport_x;
	}
	else if (addr == 0xFF44) {
		return gb.ppu.lcd_y;
	}
	else if (addr == 0xFF45) {
		return gb.ppu.ly_comp;
	}
	else if (addr == 0xFF46) {
		return gb.OAM_DMA;
	}
	else if (addr == 0xFF47) {
		return gb.ppu.bg_palette;
	}
	else if (addr == 0xFF48) {
		return gb.ppu.obj_palette0;
	}
	else if (addr == 0xFF49) {
		return gb.ppu.obj_palette1;
	}
	else if (addr == 0xFF46) {
		return gb.ppu.win_y;
	}
	else if (addr == 0xFF46) {
		return gb.ppu.win_x;
	}
	else if (addr == 0xFF4A) {
		return gb.OAM_DMA;
	}
	else if (addr == 0xFF50) {
		return gb.cart.boot_ROM_mapping;
	}
	else if (addr >= 0xFF80 and addr <= 0xFFFE) {
		return HRAM[addr - 0xFF80];
	}
	else if (addr == 0xFFFF) {
		return gb.cpu.interrupt_enable;
	}
	else {
		//Unmapped addresss
		return 0xFF;
	}

}


void MMU::write(uint16_t addr, uint8_t byte) {
	//Writes to special places like Cartrdige ROM/RAM, IO, IE, need to go to their respective class members
	//Remove all IO registers from MMU
	//Use switch case for IO registers
	/*
	case FF10:
		gb.apu.NR10 = byte
	case FF11:
		gb.apu.NR11 = byte
	...
	
	*/
}