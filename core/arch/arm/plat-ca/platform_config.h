/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2024, Cortina Access
 */

#ifndef PLATFORM_CONFIG_H
#define PLATFORM_CONFIG_H

#include <mm/generic_ram_layout.h>

/* Make stacks aligned to data cache line length */
#define STACK_ALIGNMENT		64

/* UART */
#if defined(PLATFORM_FLAVOR_mercury)

#define CA_UART0_BASE	0xF8015000
#define CA_UART1_BASE	0xF7112000
#if (CFG_CONSOLE_UART == 1)
#define CONSOLE_UART_BASE	CA_UART1_BASE
#elif (CFG_CONSOLE_UART == 0)
#define CONSOLE_UART_BASE	CA_UART0_BASE
#else
#error Unknown console UART
#endif

#else /* PLATFORM_FLAVOR_hikey */
#error Unknown Cortina PLATFORM_FLAVOR
#endif /* PLATFORM_FLAVOR_hikey */

#define CONSOLE_BAUDRATE	115200
#define CONSOLE_UART_CLK_IN_HZ	19200000

/*
 * Mercury memory map
 *
 * Refer to the default configuration from conf.mk for description below.
 *
 * TZDRAM is secured (firewalled) by the DDR controller, see ARM-TF, but note
 * that security of this type of memory is weak for two reasons:
 *   1. It is prone to physical tampering since DRAM is external to the SoC
 *   2. It is still somewhat prone to software attacks because the memory
 *      protection may be reverted by the non-secure kernel with a piece of
 *      code similar to the one that sets the protection in ARM-TF (we're
 *      missing a "lockdown" step which would prevent any change to the DDRC
 *      configuration until the next SoC reset).
 * TZSRAM is emulated in the TZDRAM area, because the on-chip SRAM of the
 * SoC is too small to run OP-TEE (72K total with 64K available, see
 * "SRAM Memory Region Layout" in ARM-TF plat/hikey/include/hisi_sram_map.h),
 * while the SRAM of the HiKey960 SoC is not available to the public at the
 * moment.
 *
 * CFG_WITH_PAGER=n
 *
 *  0x4000_0000                               -
 *    TA RAM: 14 MiB                          |
 *  0x3F20_0000                               | TZDRAM
 *    TEE RAM: 2 MiB (TEE_RAM_VA_SIZE)	      |
 *  0x3F00_0000 [TZDRAM_BASE, BL32_LOAD_ADDR] -
 *    Shared memory: 2 MiB                    |
 *  0x3EE0_0000                               | DRAM0
 *    Reserved by UEFI for OP-TEE, unused     |
 *  0x3EC0_0000                               -
 *    Secure Data Path buffers: 4 MiB         | DRAM0 (secure)
 *  0x3E80_0000 [CFG_TEE_SDP_MEM_BASE]        -
 *    Reserved by UEFI for OP-TEE, unused     |
 *  0x3E00_0000                               | DRAM0
 *    Available to Linux                      |
 *  0x0000_0000 [DRAM0_BASE]                  -
 *
 * CFG_WITH_PAGER=y
 *
 *  0x4000_0000                               -
 *    TA RAM: 14 MiB                          | TZDRAM
 *  0x3F20_0000                               -
 *    Unused
 *  0x3F03_2000                               -
 *    TEE RAM: 200 KiB                        | TZSRAM
 *  0x3F00_0000 [TZSRAM_BASE, BL32_LOAD_ADDR] -
 *    Shared memory: 2 MiB                    |
 *  0x3EE0_0000                               | DRAM0
 *    Reserved by UEFI for OP-TEE, unused     |
 *  0x3EC0_0000                               -
 *    Secure Data Path buffers: 4 MiB         | DRAM0 (secure)
 *  0x3E80_0000 [CFG_TEE_SDP_MEM_BASE]        -
 *    Reserved by UEFI for OP-TEE, unused     |
 *  0x3E00_0000                               | DRAM0
 *    Available to Linux                      |
 *  0x0000_0000 [DRAM0_BASE]                  -
 */

#define DRAM0_BASE		0x00000000
/* DRAM0 exists below TZDRAM */
#define DRAM0_SIZE		(CFG_TZDRAM_START - DRAM0_BASE)
#define DRAM0_SIZE_NSEC		0x3E000000
#define DRAM1_BASE		CFG_DRAM1_BASE

#if defined(PLATFORM_FLAVOR_mercury)

#if (CFG_DRAM_SIZE_GB == 2)
#define DRAM1_SIZE_NSEC		0x40000000
#elif (CFG_DRAM_SIZE_GB == 1)
/* do nothing */
#else
#error Unknown DRAM size
#endif


#else /* PLATFORM_FLAVOR_mercury */
#error Unknown CA PLATFORM_FLAVOR
#endif /* PLATFORM_FLAVOR_mercury */

#endif /* PLATFORM_CONFIG_H */
