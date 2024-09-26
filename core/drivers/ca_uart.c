// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2024, Cortina Access Inc.
 */
#include <assert.h>
#include <drivers/ca_uart.h>
#include <io.h>
#include <util.h>
#include <keep.h>
#include <kernel/dt.h>
#include <kernel/dt_driver.h>
#include <kernel/spinlock.h>
#include <stdlib.h>
#include <trace.h>
#include <types_ext.h>
#include <util.h>


static vaddr_t chip_to_base(struct serial_chip *chip)
{
	struct ca_uart_data *pd =
		container_of(chip, struct ca_uart_data, chip);

	return io_pa_or_va(&pd->base, CA_UART_REG_SIZE);
}

static void ca_uart_flush(struct serial_chip *chip)
{
	vaddr_t base = chip_to_base(chip);

	/*
	 * Wait for the transmit FIFO to be empty.
	 * It can happen that Linux initializes the OP-TEE driver with the
	 * console UART disabled; avoid an infinite loop by checking the UART
	 * enabled flag. Checking it in the loop makes the code safe against
	 * asynchronous disable.
	 */

	while ((io_read32(base + CFG) & (CFG_UART_EN | CFG_TX_EN | CFG_RX_EN)) &&
 		( !(io_read32(base + INFO) & INFO_TX_EMPTY)))
		;
}

static bool ca_uart_have_rx_data(struct serial_chip *chip)
{
	vaddr_t base = chip_to_base(chip);

	return !(io_read32(base + INFO) & INFO_RX_EMPTY);
}

static int ca_uart_getchar(struct serial_chip *chip)
{
	vaddr_t base = chip_to_base(chip);

	while (!ca_uart_have_rx_data(chip))
		;
	return(io_read32(base + UART_DR) & 0xff);
}

static void ca_uart_putc(struct serial_chip *chip, int ch)
{
	vaddr_t base = chip_to_base(chip);

	/* wait for transmitter to empty to accept a character */
	while (!io_read32(base + INFO) & INFO_TX_EMPTY)
		;
	/* Send the character */
	io_write32(base + TX_DAT, ch);
}

static void ca_uart_rx_intr_enable(struct serial_chip *chip)
{
	vaddr_t base = chip_to_base(chip);
	uint32_t temp;

        temp = io_read32(base + IE);
        temp |= UINT_RX_ALL;
        io_write32(port->membase + IE, temp);
}

static void ca_uart_rx_intr_disable(struct serial_chip *chip)
{
	vaddr_t base = chip_to_base(chip);
	uint32_t temp;

        temp = io_read32(base + IE);
	temp &= ~(UINT_RX_ALL);
        io_write32(port->membase + IE, temp);
}

static const struct serial_ops ca_uart_ops = {
	.flush = ca_uart_flush,
	.getchar = ca_uart_getchar,
	.have_rx_data = ca_uart_have_rx_data,
	.putc = ca_uart_putc,
	.rx_intr_enable = ca_uart_rx_intr_enable,
	.rx_intr_disable = ca_uart_rx_intr_disable,
};
DECLARE_KEEP_PAGER(ca_uart_ops);

void ca_uart_init(struct ca_uart_data *pd, paddr_t pbase, uint32_t uart_clk,
		uint32_t baud_rate)
{
	vaddr_t base;

	pd->base.pa = pbase;
	pd->chip.ops = &ca_uart_ops;

	base = io_pa_or_va(&pd->base, CA_UART_REG_SIZE);

        uint32_t temp; /* temp cfg */
        uint32_t sample_fre = 0;

	/* Turn off Tx interrupts. The port lock is held at this point */
        temp = io_read32(base + IE);
        temp &= ~(UINT_TX_ALL);
        io_write32(membase + IE, temp);

        /* Turn off Rx interrupts. The port lock is held at this point */
        temp = io_read32(base + IE);
        temp &= ~(UINT_RX_ALL);
        io_write32(base + IE, temp);


        if ((baud_rate != 0) && (uart_clk != 0)) {

        	temp = io_read32(base + CFG);
                /* mask off the baud settings held in the top 24 bits */
                temp &= 0xff;
                dev_data->baud_rate = baud_rate;

                switch (baud_rate) {
                 case 9600:
                        temp |= (uart_clk / 9600) << CFG_BAUD_SART ;
                        break;
                 case 19200:
                        temp |= (uart_clk / 19200) << CFG_BAUD_SART ;
                        break;
                 case 38400:
                        temp |= (uart_clk / 38400) << CFG_BAUD_SART ;
                        break;
                 case 57600:
                        temp |= (uart_clk / 57600) << CFG_BAUD_SART ;
                        break;
                 case 115200:
                        temp |= (uart_clk / 115200) << CFG_BAUD_SART ;
                        break;
                 default:
                        temp |= (uart_clk / 38400) << CFG_BAUD_SART ;
                        break;
                }

                /* Sampling rate should be half of baud count */
                sample_fre = (temp >> CFG_BAUD_SART) / 2;
                io_write32(base + CFG, temp);
                io_write32(base + RX_SAMPLE, sample_fre);
        }

	/* set 8,N,1 */
        temp = io_read32(base + CFG);

	/* 8 data bits */
        /* mask off the data width */
        temp &= 0xfffffffc;
        temp |= 0x3;

        /* 1 stop bit */
        /* mask off the 2 Stop enable bit */
        temp &= ~(CFG_STOP_2BIT);

        /* no parity */
        /* mask off the parity enable bit */
        temp &= ~(CFG_PARITY_EN);
        io_write32(base + CFG, temp);

	/* Enable UART and RX/TX */
        temp = io_read32(base + CFG);
        io_write32(base + CFG, (temp | CFG_UART_EN | CFG_TX_EN | CFG_RX_EN));

	ca_uart_flush(&pd->chip);
}

#ifdef CFG_DT

static struct serial_chip *ca_uart_dev_alloc(void)
{
	struct ca_uart_data *pd = nex_calloc(1, sizeof(*pd));

	if (!pd)
		return NULL;
	return &pd->chip;
}

static int ca_uart_dev_init(struct serial_chip *chip, const void *fdt, int offs,
			  const char *parms)
{
	struct ca_uart_data *pd = container_of(chip, struct ca_uart_data, chip);
	vaddr_t vbase;
	paddr_t pbase;
	size_t size;
	uint32_t uart_clock;
	const fdt32_t *cuint = NULL;

	if (parms && parms[0])
		IMSG("ca_uart: device parameters ignored (%s)", parms);

	if (dt_map_dev(fdt, offs, &vbase, &size, DT_MAP_AUTO) < 0)
		return -1;

	if (size != 0x1000) {
		EMSG("ca_uart: unexpected register size: %zx", size);
		return -1;
	}

	cuint = fdt_getprop(fdt, node, "clocks", NULL);
        if (cuint) {
                uart_clock = fdt32_to_cpu(*cuint);
		IMSG("ca_uart: uart_clock is (%zx)", uart_clock);

        } else {
		EMSG("ca_uart: uart_clock not found in DTS");
		return -1;
        }

	pbase = virt_to_phys((void *)vbase);
	ca_uart_init(pd, pbase, 115200, uart_clock);

	return 0;
}

static void ca_uart_dev_free(struct serial_chip *chip)
{
	struct ca_uart_data *pd = container_of(chip, struct ca_uart_data, chip);

	nex_free(pd);
}

static const struct serial_driver ca_uart_driver = {
	.dev_alloc = ca_uart_dev_alloc,
	.dev_init = ca_uart_dev_init,
	.dev_free = ca_uart_dev_free,
};

static const struct dt_device_match ca_uart_match_table[] = {
	{ .compatible = "cortina-access,serial" },
	{ 0 }
};

DEFINE_DT_DRIVER(ca_uart_dt_driver) = {
	.name = "ca_uart",
	.type = DT_DRIVER_UART,
	.match_table = ca_uart_match_table,
	.driver = &ca_uart_driver,
};

#endif /* CFG_DT */
