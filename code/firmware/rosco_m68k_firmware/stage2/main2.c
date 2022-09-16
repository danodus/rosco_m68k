/*
 *------------------------------------------------------------
 *                                  ___ ___ _   
 *  ___ ___ ___ ___ ___       _____|  _| . | |_ 
 * |  _| . |_ -|  _| . |     |     | . | . | '_|
 * |_| |___|___|___|___|_____|_|_|_|___|___|_,_| 
 *                     |_____|       firmware v2
 * ------------------------------------------------------------
 * Copyright (c)2019-2021 Ross Bamford and contributors
 * See top-level LICENSE.md for licence information.
 *
 * Entry point to ROM stage 2
 * ------------------------------------------------------------
 */

#include <stddef.h>
#include <stdint.h>
#include <stdnoreturn.h>
#include <stdbool.h>
#include "machine.h"
#include "system.h"
#include "serial.h"
#include "rtlsupport.h"

extern void mcPrint(char*);
extern void mcHalt();
extern void ENABLE_RECV();
extern void red_led_off();

#ifdef KERMIT_LOADER
extern void mcBusywait(uint32_t);
#endif

// This is what a Kernel entry point should look like
typedef void (*KMain)(volatile SystemDataBlock * const);

// Linker defines
extern uint32_t _bss_start, _bss_end;
static volatile SystemDataBlock * const sdb = (volatile SystemDataBlock * const)0x400;

// Kernels are loaded at the same address regardless of _how_ they're loaded
uint8_t *kernel_load_ptr = (uint8_t*) KERNEL_LOAD_ADDRESS;
static KMain kmain = (KMain) KERNEL_LOAD_ADDRESS;

#ifdef KERMIT_LOADER
// This is provided by Kermit
extern int receive_kernel();
#endif

#ifdef BLOCKDEV_LOADER
// This is provided by the SD/FAT loader
extern bool sd_load_kernel();
// This is provided by the IDE/FAT loader
extern bool ide_load_kernel();
#endif

void linit() {
    // zero .bss
    for (uint32_t *dst = &_bss_start; dst < &_bss_end; *dst = 0, dst++);
}

noreturn void lmain() {
    // Always do this for backwards compatibility
    ENABLE_RECV();

#ifdef MAME_LOADER
    mcPrint("Starting MAME Quickload kernel...\r\n");
    goto have_kernel;
#endif

#ifdef BLOCKDEV_LOADER
    mcPrint("Searching for boot media...\r\n");

    if (sd_load_kernel()) {
        goto have_kernel;
    }

    if (ide_load_kernel()) {
        goto have_kernel;
    }

    mcPrint("No bootable media found\r\n");
#endif

#ifdef KERMIT_LOADER
    mcPrint("Ready for Kermit receive...\r\n");

    mcBusywait(100000);

    while (!receive_kernel()) {
        mcPrint("\x1b[1;31mSEVERE\x1b[0m: Receive failed; Ready for retry...\r\n");
    }

    // Wait a short while for the user's terminal to come back...
    mcBusywait(400000);
    
    mcPrint("Kernel received okay; Starting...\r\n");
    goto have_kernel;
#endif

    mcPrint("No bootable media found & no Kermit support; Halting...\r\n");
    goto halt;

have_kernel:
    red_led_off();
    mcPrint("\r\n");

    kmain(sdb);

    mcPrint("\x1b[1;31mSEVERE\x1b: Kernel should not return! Halting\r\n");

halt:
    while (true) {
        mcHalt();
    }
}
