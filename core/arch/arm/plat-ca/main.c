// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2015, Linaro Limited
 */

#include <console.h>
#ifdef CFG_CA_UART
#include <drivers/ca_uart.h>
#endif
#if defined(PLATFORM_FLAVOR_mercury)
#include <mercury_peripherals.h>
#endif
#include <initcall.h>
#include <io.h>
#include <kernel/panic.h>
#include <mm/tee_pager.h>
#include <mm/core_memprot.h>
#include <platform_config.h>
#include <stdint.h>

static struct ca_uart_data console_data __nex_bss;

/* non secure mappings */
register_phys_mem_pgdir(MEM_AREA_IO_NSEC, CONSOLE_UART_BASE, CA_UART_REG_SIZE);

#ifdef DRAM1_SIZE_NSEC
register_ddr(DRAM0_BASE, DRAM0_SIZE_NSEC);

#ifdef DRAM1_SIZE_NSEC
register_ddr(DRAM1_BASE, DRAM1_SIZE_NSEC);
#endif
#ifdef DRAM2_SIZE_NSEC
register_ddr(DRAM2_BASE, DRAM2_SIZE_NSEC);
#endif

void plat_console_init(void)
{
	ca_uart_init(&console_data, CONSOLE_UART_BASE, CONSOLE_UART_CLK_IN_HZ,
		   CONSOLE_BAUDRATE);
	register_serial_console(&console_data.chip);
}

#if defined(PLATFORM_FLAVOR_mercury)

static TEE_Result peripherals_init(void)
{
	return TEE_SUCCESS;
}

driver_init(peripherals_init);
#endif /* PLATFORM_FLAVOR_mercury */
