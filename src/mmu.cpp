#include "mmu.h"
#include "gb.h"
#include "cstring"

MMU::MMU(GB* in_gb) :
	gb(in_gb)
{
	memset(VRAM, 0, sizeof(VRAM));
	memset(WRAM1, 0, sizeof(WRAM1));
	memset(WRAM2, 0, sizeof(WRAM2));
	memset(OAM, 0, sizeof(OAM));
	memset(HRAM, 0, sizeof(HRAM));
}

uint8_t* MMU::ptr(uint16_t addr) {
	gb->tick_other_components();

	if (addr >= 0x0000 and addr <= 0x7FFF) {
		return gb->cart.ptr_ROM_bank(addr);
	}
	else if (addr >= 0x8000 and addr <= 0x9FFF) {
		return &VRAM[addr - 0x8000];
	}
	else if (addr >= 0xA000 and addr <= 0xBFFF) {
		return gb->cart.ptr_RAM(addr - 0xA000);
	}
	else if (addr >= 0xC000 and addr <= 0xCFFF) {
		return &WRAM1[addr - 0xC000];
	}
	else if (addr >= 0xD000 and addr <= 0xDFFF) {
		return &WRAM2[addr - 0xD000];
	}
	else if (addr >= 0xE000 and addr <= 0xFDFF) {
		//Echo ram unimplemented
		return nullptr;
	}
	else if (addr >= 0xFE00 and addr <= 0xFD9F) {
		return &OAM[addr - 0xFE00];
	}
	else if (addr >= 0xFEA0 and addr <= 0xFEFF) {
		//Should return 0xFF during OAM block
		return nullptr;
	}
	else if (addr == 0xFF00) {
		return gb->input.ptr_joypad();
	}
	else if (addr == 0xFF04) {
		return &gb->timer.DIV;
	}
	else if (addr == 0xFF05) {
		return &gb->timer.TIMA;
	}
	else if (addr == 0xFF06) {
		return &gb->timer.TMA;
	}
	else if (addr == 0xFF07) {
		return &gb->timer.TAC;
	}
	else if (addr == 0xFF0F) {
		return &gb->cpu.interrupt_flag;
	}
	else if (addr == 0xFF10) {
		return &gb->apu.NR10;
	}
	else if (addr == 0xFF11) {
		return &gb->apu.NR11;
	}
	else if (addr == 0xFF12) {
		return &gb->apu.NR12;
	}
	else if (addr == 0xFF13) {
		return &gb->apu.NR13;
	}
	else if (addr == 0xFF14) {
		return &gb->apu.NR14;
	}
	else if (addr == 0xFF16) {
		return &gb->apu.NR21;
	}
	else if (addr == 0xFF17) {
		return &gb->apu.NR22;
	}
	else if (addr == 0xFF18) {
		return &gb->apu.NR23;
	}
	else if (addr == 0xFF19) {
		return &gb->apu.NR24;
	}
	else if (addr == 0xFF1A) {
		return &gb->apu.NR30;
	}
	else if (addr == 0xFF1B) {
		return &gb->apu.NR31;
	}
	else if (addr == 0xFF1C) {
		return &gb->apu.NR32;
	}
	else if (addr == 0xFF1D) {
		return &gb->apu.NR33;
	}
	else if (addr == 0xFF1E) {
		return &gb->apu.NR34;
	}
	else if (addr == 0xFF20) {
		return &gb->apu.NR41;
	}
	else if (addr == 0xFF21) {
		return &gb->apu.NR42;
	}
	else if (addr == 0xFF22) {
		return &gb->apu.NR43;
	}
	else if (addr == 0xFF23) {
		return &gb->apu.NR44;
	}
	else if (addr == 0xFF24) {
		return &gb->apu.NR50;
	}
	else if (addr == 0xFF25) {
		return &gb->apu.NR51;
	}
	else if (addr == 0xFF26) {
		return &gb->apu.NR52;
	}
	else if (addr >= 0xFF30 and addr <= 0xFF3F) {
		return &gb->apu.wave[addr - 0xFF30];
	}
	else if (addr == 0xFF40) {
		return &gb->ppu.lcd_control;
	}
	else if (addr == 0xFF41) {
		return &gb->ppu.lcd_status;
	}
	else if (addr == 0xFF42) {
		return &gb->ppu.bg_viewport_y;
	}
	else if (addr == 0xFF43) {
		return &gb->ppu.bg_viewport_x;
	}
	else if (addr == 0xFF44) {
		return &gb->ppu.lcd_y;
	}
	else if (addr == 0xFF45) {
		return &gb->ppu.ly_comp;
	}
	else if (addr == 0xFF46) {
		return &gb->OAM_DMA;
	}
	else if (addr == 0xFF47) {
		return &gb->ppu.bg_palette;
	}
	else if (addr == 0xFF48) {
		return &gb->ppu.obj_palette0;
	}
	else if (addr == 0xFF49) {
		return &gb->ppu.obj_palette1;
	}
	else if (addr == 0xFF4A) {
		return &gb->ppu.win_y;
	}
	else if (addr == 0xFF4B) {
		return &gb->ppu.win_x;
	}
	else if (addr == 0xFF50) {
		return &gb->cart.boot_ROM_mapping;
	}
	else if (addr >= 0xFF80 and addr <= 0xFFFE) {
		return &HRAM[addr - 0xFF80];
	}
	else if (addr == 0xFFFF) {
		return &gb->cpu.interrupt_enable;
	}
	else {
		//Unmapped addresss
		return nullptr;
	}
}

uint8_t MMU::read(uint16_t addr) {
	return *ptr(addr);
}

void MMU::write(uint16_t addr, uint8_t byte) {
	gb->tick_other_components();

	if (addr >= 0x0000 && addr <= 0x7FFF) {
		gb->cart.write_ROM_bank(addr, byte);
	}
	else if (addr >= 0x8000 && addr <= 0x9FFF) {
		VRAM[addr - 0x8000] = byte;
	}
	else if (addr >= 0xA000 && addr <= 0xBFFF) {
		gb->cart.write_RAM(addr - 0xA000, byte);
	}
	else if (addr >= 0xC000 && addr <= 0xCFFF) {
		WRAM1[addr - 0xC000] = byte;
	}
	else if (addr >= 0xD000 && addr <= 0xDFFF) {
		WRAM2[addr - 0xD000] = byte;
	}
	else if (addr >= 0xFE00 && addr <= 0xFE9F) {
		OAM[addr - 0xFE00] = byte;
	}
	else if (addr == 0xFF00) {
		gb->input.write_joypad(byte);
	}
	else if (addr == 0xFF04) {
		//Writing any byte resets the DIV register
		gb->timer.DIV =0;  
	}
	else if (addr == 0xFF05) {
		gb->timer.TIMA = byte;
	}
	else if (addr == 0xFF06) {
		gb->timer.TMA = byte;
	}
	else if (addr == 0xFF07) {
		gb->timer.TAC = byte;
	}
	else if (addr == 0xFF0F) {
		gb->cpu.interrupt_flag = byte;
	}
	else if (addr == 0xFF10) {
		gb->apu.NR10 = byte;
	}
	else if (addr == 0xFF11) {
		gb->apu.NR11 = byte;
	}
	else if (addr == 0xFF12) {
		gb->apu.NR12 = byte;
	}
	else if (addr == 0xFF13) {
		gb->apu.NR13 = byte;
	}
	else if (addr == 0xFF14) {
		gb->apu.NR14 = byte;
	}
	else if (addr == 0xFF16) {
		gb->apu.NR21 = byte;
	}
	else if (addr == 0xFF17) {
		gb->apu.NR22 = byte;
	}
	else if (addr == 0xFF18) {
		gb->apu.NR23 = byte;
	}
	else if (addr == 0xFF19) {
		gb->apu.NR24 = byte;
	}
	else if (addr == 0xFF1A) {
		gb->apu.NR30 = byte;
	}
	else if (addr == 0xFF1B) {
		gb->apu.NR31 = byte;
	}
	else if (addr == 0xFF1C) {
		gb->apu.NR32 = byte;
	}
	else if (addr == 0xFF1D) {
		gb->apu.NR33 = byte;
	}
	else if (addr == 0xFF1E) {
		gb->apu.NR34 = byte;
	}
	else if (addr == 0xFF20) {
		gb->apu.NR41 = byte;
	}
	else if (addr == 0xFF21) {
		gb->apu.NR42 = byte;
	}
	else if (addr == 0xFF22) {
		gb->apu.NR43 = byte;
	}
	else if (addr == 0xFF23) {
		gb->apu.NR44 = byte;
	}
	else if (addr == 0xFF24) {
		gb->apu.NR50 = byte;
	}
	else if (addr == 0xFF25) {
		gb->apu.NR51 = byte;
	}
	else if (addr == 0xFF26) {
		gb->apu.NR52 = byte;
	}
	else if (addr >= 0xFF30 && addr <= 0xFF3F) {
		gb->apu.wave[addr - 0xFF30] = byte;
	}
	else if (addr == 0xFF40) {
		gb->ppu.lcd_control = byte;
	}
	else if (addr == 0xFF41) {
		gb->ppu.lcd_status = byte;
	}
	else if (addr == 0xFF42) {
		gb->ppu.bg_viewport_y = byte;
	}
	else if (addr == 0xFF43) {
		gb->ppu.bg_viewport_x = byte;
	}
	else if (addr == 0xFF44) {
		//lcd_y read only
	}
	else if (addr == 0xFF45) {
		gb->ppu.ly_comp = byte;
	}
	else if (addr == 0xFF46) {
		gb->OAM_DMA = byte;
		gb->ppu.start_OAM_DMA(byte);
	}
	else if (addr == 0xFF47) {
		gb->ppu.bg_palette = byte;
	}
	else if (addr == 0xFF48) {
		gb->ppu.obj_palette0 = byte;
	}
	else if (addr == 0xFF49) {
		gb->ppu.obj_palette1 = byte;
	}
	else if (addr == 0xFF4A) {
		gb->ppu.win_y = byte;
	}
	else if (addr == 0xFF4B) {
		gb->ppu.win_x = byte;
	}
	else if (addr == 0xFF50) {
		gb->cart.boot_ROM_mapping = byte;
	}
	else if (addr >= 0xFF80 && addr <= 0xFFFE) {
		HRAM[addr - 0xFF80] = byte;
	}
	else if (addr == 0xFFFF) {
		gb->cpu.interrupt_enable = byte;
	}
	else {
		//Unmapped address
	}
}