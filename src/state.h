#ifndef _STATE_H
#define _STATE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "wd279x.h"

// Maximum size of the Boot PROMs. Must be a binary power of two.
#define ROM_SIZE 32768

/**
 * @brief Emulator state storage
 *
 * This structure stores the internal state of the emulator.
 */
typedef struct {
	// Boot PROM can be up to 32Kbytes total size
	uint8_t		rom[ROM_SIZE];		///< Boot PROM data buffer

	//// Main system RAM
	uint8_t		*ram;				///< RAM data buffer
	size_t		ram_size;			///< Size of RAM buffer in bytes

	/// Video RAM
	uint8_t		vram[0x8000];

	/// Map RAM
	uint8_t		map[0x800];

	//// Registers
	uint16_t	genstat;			///< General Status Register
	uint16_t	bsr0;				///< Bus Status Register 0
	uint16_t	bsr1;				///< Bus Status Register 1

	//// MISCELLANEOUS CONTROL REGISTER
	bool		dma_reading;		///< True if Disc DMA reads from the controller, false otherwise
	uint8_t		leds;				///< LED status, 1=on, in order red3/green2/yellow1/red0 from bit3 to bit0

	//// GENERAL CONTROL REGISTER
	/// GENCON.ROMLMAP -- false ORs the address with 0x800000, forcing the
	/// 68010 to access ROM instead of RAM when booting. TRM page 2-36.
	bool		romlmap;
	/// GENCON.PIE -- Parity Error Check Enable
	bool		pie;

	/// DMA Address Register
	uint32_t	dma_address;

	/// Floppy disc controller context
	WD2797_CTX	fdc_ctx;
} S_state;

// Global emulator state. Yes, I know global variables are evil, please don't
// email me and lecture me about it.  -philpem
#ifndef _STATE_C
extern S_state state;
#else
S_state state;
#endif

/**
 * @brief	Initialise system state
 *
 * @param	ramsize		RAM size in bytes -- must be a multiple of 512KiB, min 512KiB, max 4MiB.
 *
 * Initialises the emulator's internal state.
 */
int state_init(size_t ramsize);

/**
 * @brief Deinitialise system state
 *
 * Deinitialises the saved state, and frees all memory. Call this function
 * before exiting your program to avoid memory leaks.
 */
void state_done();

#endif
