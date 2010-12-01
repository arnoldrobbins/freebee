#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <malloc.h>
#include <string.h>

#include "SDL.h"

#include "musashi/m68k.h"
#include "version.h"
#include "state.h"

void FAIL(char *err)
{
	state_done();
	fprintf(stderr, "ERROR: %s\nExiting...\n", err);
	exit(EXIT_FAILURE);
}

/***********************************
 * Array read/write utility macros
 * "Don't Repeat Yourself" :)
 ***********************************/

/// Array read, 32-bit
#define RD32(array, address, andmask)							\
	(((uint32_t)array[(address + 0) & (andmask)] << 24) |		\
	 ((uint32_t)array[(address + 1) & (andmask)] << 16) |		\
	 ((uint32_t)array[(address + 2) & (andmask)] << 8)  |		\
	 ((uint32_t)array[(address + 3) & (andmask)]))

/// Array read, 16-bit
#define RD16(array, address, andmask)							\
	(((uint32_t)array[(address + 0) & (andmask)] << 8)  |		\
	 ((uint32_t)array[(address + 1) & (andmask)]))

/// Array read, 8-bit
#define RD8(array, address, andmask)							\
	((uint32_t)array[(address + 0) & (andmask)])

/// Array write, 32-bit
#define WR32(array, address, andmask, value) {					\
	array[(address + 0) & (andmask)] = (value >> 24) & 0xff;	\
	array[(address + 1) & (andmask)] = (value >> 16) & 0xff;	\
	array[(address + 2) & (andmask)] = (value >> 8)  & 0xff;	\
	array[(address + 3) & (andmask)] =  value        & 0xff;	\
}

/// Array write, 16-bit
#define WR16(array, address, andmask, value) {					\
	array[(address + 0) & (andmask)] = (value >> 8)  & 0xff;	\
	array[(address + 1) & (andmask)] =  value        & 0xff;	\
}

/// Array write, 8-bit
#define WR8(array, address, andmask, value)						\
	array[(address + 0) & (andmask)] =  value        & 0xff;


/********************************************************
 * m68k memory read/write support functions for Musashi
 ********************************************************/


// read m68k memory
uint32_t m68k_read_memory_32(uint32_t address)
{
	uint32_t data = 0xFFFFFFFF;

	// If ROMLMAP is set, force system to access ROM
	if (!state.romlmap)
		address |= 0x800000;

	if ((address >= 0x800000) && (address <= 0xBFFFFF)) {
		// ROM access
		data = RD32(state.rom, address, ROM_SIZE - 1);
	} else if (address <= (state.ram_size - 1)) {
		// RAM
		data = RD32(state.ram, address, state.ram_size - 1);
	} else if ((address >= 0x420000) && (address <= 0x427FFF)) {
		// VRAM
		data = RD32(state.vram, address, 0x7FFF);
	} else {
		// I/O register -- TODO
		printf("RD32 0x%08X [unknown I/O register]\n", address);
	}
	return data;
}

uint32_t m68k_read_memory_16(uint32_t address)
{
	uint16_t data = 0xFFFF;

	// If ROMLMAP is set, force system to access ROM
	if (!state.romlmap)
		address |= 0x800000;

	if ((address >= 0x800000) && (address <= 0xBFFFFF)) {
		// ROM access
		data = RD16(state.rom, address, ROM_SIZE - 1);
	} else if (address <= (state.ram_size - 1)) {
		// RAM
		data = RD16(state.ram, address, state.ram_size - 1);
	} else if ((address >= 0x420000) && (address <= 0x427FFF)) {
		// VRAM
		data = RD16(state.vram, address, 0x7FFF);
	} else {
		// I/O register -- TODO
		printf("RD16 0x%08X [unknown I/O register]\n", address);
	}

	return data;
}

uint32_t m68k_read_memory_8(uint32_t address)
{
	uint8_t data = 0xFF;

	// If ROMLMAP is set, force system to access ROM
	if (!state.romlmap)
		address |= 0x800000;

	if ((address >= 0x800000) && (address <= 0xBFFFFF)) {
		// ROM access
		data = RD8(state.rom, address, ROM_SIZE - 1);
	} else if (address <= (state.ram_size - 1)) {
		// RAM
		data = RD8(state.ram, address, state.ram_size - 1);
	} else if ((address >= 0x420000) && (address <= 0x427FFF)) {
		// VRAM
		data = RD8(state.vram, address, 0x7FFF);
	} else {
		// I/O register -- TODO
		printf("RD08 0x%08X [unknown I/O register]\n", address);
	}

	return data;
}

// write m68k memory
void m68k_write_memory_32(uint32_t address, uint32_t value)
{
	// If ROMLMAP is set, force system to access ROM
	if (!state.romlmap)
		address |= 0x800000;

	if ((address >= 0x800000) && (address <= 0xBFFFFF)) {
		// ROM access
		// TODO: bus error here? can't write to rom!
	} else if (address <= (state.ram_size - 1)) {
		// RAM access
		WR32(state.ram, address, state.ram_size - 1, value);
	} else if ((address >= 0x420000) && (address <= 0x427FFF)) {
		// VRAM access
		WR32(state.vram, address, 0x7fff, value);
	} else {
		switch (address) {
			case 0xE43000:	state.romlmap = ((value & 0x8000) == 0x8000); break;	// GCR3: ROMLMAP
			default:		printf("WR32 0x%08X ==> 0x%08X\n", address, value); break;
		}
	}
}

void m68k_write_memory_16(uint32_t address, uint32_t value)
{
	// If ROMLMAP is set, force system to access ROM
	if (!state.romlmap)
		address |= 0x800000;

	if ((address >= 0x800000) && (address <= 0xBFFFFF)) {
		// ROM access
		// TODO: bus error here? can't write to rom!
	} else if (address <= (state.ram_size - 1)) {
		// RAM access
		WR16(state.ram, address, state.ram_size - 1, value);
	} else if ((address >= 0x420000) && (address <= 0x427FFF)) {
		// VRAM access
		WR16(state.vram, address, 0x7fff, value);
	} else {
		switch (address) {
			case 0xE43000:	state.romlmap = ((value & 0x8000) == 0x8000); break;	// GCR3: ROMLMAP
			default:		printf("WR16 0x%08X ==> 0x%04X\n", address, value); break;
		}
		if (address == 0x4A0000) {
			printf("\tLED WRITE: %s %s %s %s\n",
					value & 0x800 ? "-" : "R",
					value & 0x400 ? "-" : "G",
					value & 0x200 ? "-" : "Y",
					value & 0x100 ? "-" : "R"
					);
		}
	}
}

void m68k_write_memory_8(uint32_t address, uint32_t value)
{
	// If ROMLMAP is set, force system to access ROM
	if (!state.romlmap)
		address |= 0x800000;

	if ((address >= 0x800000) && (address <= 0xBFFFFF)) {
		// ROM access
		// TODO: bus error here? can't write to rom!
	} else if (address <= (state.ram_size - 1)) {
		// RAM access
		WR8(state.ram, address, state.ram_size - 1, value);
	} else if ((address >= 0x420000) && (address <= 0x427FFF)) {
		// VRAM access
		WR8(state.vram, address, 0x7fff, value);
	} else {
		switch (address) {
			case 0xE43000:	state.romlmap = ((value & 0x80) == 0x80); break;	// GCR3: ROMLMAP
			default:		printf("WR08 0x%08X ==> 0x%02X\n", address, value); break;
		}
	}
}

// for the disassembler
uint32_t m68k_read_disassembler_32(uint32_t addr) { return m68k_read_memory_32(addr); }
uint32_t m68k_read_disassembler_16(uint32_t addr) { return m68k_read_memory_16(addr); }
uint32_t m68k_read_disassembler_8 (uint32_t addr) { return m68k_read_memory_8 (addr); }

int main(void)
{
	// copyright banner
	printf("FreeBee: A Quick-and-Dirty AT&T 3B1 Emulator. Version %s, %s mode.\n", VER_FULLSTR, VER_BUILD_TYPE);
	printf("Copyright (C) 2010 P. A. Pemberton. All rights reserved.\nLicensed under the Apache License Version 2.0.\n");
	printf("Musashi M680x0 emulator engine developed by Karl Stenerud <kstenerud@gmail.com>\n");
	printf("Built %s by %s@%s.\n", VER_COMPILE_DATETIME, VER_COMPILE_BY, VER_COMPILE_HOST);
	printf("Compiler: %s\n", VER_COMPILER);
	printf("CFLAGS: %s\n", VER_CFLAGS);
	printf("\n");

	// set up system state
	// 512K of RAM
	state_init(512*1024);

	// set up musashi and reset the CPU
	m68k_set_cpu_type(M68K_CPU_TYPE_68010);
	m68k_pulse_reset();
/*
	size_t i = 0x80001a;
	size_t len;
	do {
		char dasm[512];
		len = m68k_disassemble(dasm, i, M68K_CPU_TYPE_68010);
		printf("%06X: %s\n", i, dasm);
		i += len;
	} while (i < 0x8000ff);
*/

	// set up SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1) {
		printf("Could not initialise SDL: %s.\n", SDL_GetError());
		return -1;
	}

	/***
	 * The 3B1 CPU runs at 10MHz, with DMA running at 1MHz and video refreshing at
	 * around 60Hz (???), with a 60Hz periodic interrupt.
	 */
	const uint32_t TIMESLOT_FREQUENCY = 240;	// Hz
	const uint32_t MILLISECS_PER_TIMESLOT = 1e3 / TIMESLOT_FREQUENCY;
	const uint32_t CLOCKS_PER_60HZ = (10e6 / 60);
	uint32_t next_timeslot = SDL_GetTicks() + MILLISECS_PER_TIMESLOT;
	uint32_t clock_cycles = 0;
	bool exitEmu = false;
	for (;;) {
		// Run the CPU for however many cycles we need to. CPU core clock is
		// 10MHz, and we're running at 240Hz/timeslot. Thus: 10e6/240 or
		// 41667 cycles per timeslot.
		clock_cycles += m68k_execute(10e6/TIMESLOT_FREQUENCY);

		// TODO: run DMA here

		// Is it time to run the 60Hz periodic interrupt yet?
		if (clock_cycles > CLOCKS_PER_60HZ) {
			// TODO: refresh screen
			// TODO: trigger periodic interrupt (if enabled)
			// decrement clock cycle counter, we've handled the intr.
			clock_cycles -= CLOCKS_PER_60HZ;
		}

		// make sure frame rate is equal to real time
		uint32_t now = SDL_GetTicks();
		if (now < next_timeslot) {
			// timeslot finished early -- eat up some time
			SDL_Delay(next_timeslot - now);
		} else {
			// timeslot finished late -- skip ahead to gain time
			// TODO: if this happens a lot, we should let the user know
			// that their PC might not be fast enough...
			next_timeslot = now;
		}
		// advance to the next timeslot
		next_timeslot += MILLISECS_PER_TIMESLOT;

		// if we've been asked to exit the emulator, then do so.
		if (exitEmu) break;
	}

	// shut down and exit
	SDL_Quit();

	return 0;
}
