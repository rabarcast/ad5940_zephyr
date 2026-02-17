/**
 * @file       ZephyrPort.c
 * @brief      AD5940 platform port for Zephyr RTOS on nRF5340 DK.
 * @details    Implements the 7+2 platform functions required by ad5940.c.
 *
 * Copyright (c) 2017-2019 Analog Devices, Inc. All Rights Reserved.
 */

#include "ad5940.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ad5940_port, LOG_LEVEL_INF);

/* ---------- Devicetree handles ---------- */

/* SPI bus (no cs-gpios — CS is managed manually by ad5940.c) */
static const struct device *spi_dev = DEVICE_DT_GET(DT_NODELABEL(spi3));

static const struct spi_config spi_cfg = {
	.frequency  = 4000000,
	.operation  = SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
};

/* GPIOs defined in the overlay under /ad5940_gpios */
static const struct gpio_dt_spec cs_gpio  = GPIO_DT_SPEC_GET(DT_NODELABEL(cs_pin),  gpios);
static const struct gpio_dt_spec rst_gpio = GPIO_DT_SPEC_GET(DT_NODELABEL(rst_pin), gpios);
static const struct gpio_dt_spec int_gpio = GPIO_DT_SPEC_GET(DT_NODELABEL(int_pin), gpios);

static struct gpio_callback int_cb_data;

/* ---------- Interrupt flag ---------- */

volatile static uint8_t ucInterrupted;

static void gp0int_isr(const struct device *dev, struct gpio_callback *cb,
			uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);
	ucInterrupted = 1;
}

/* ---------- Platform functions required by ad5940.c ---------- */

void AD5940_ReadWriteNBytes(unsigned char *pSendBuffer,
			    unsigned char *pRecvBuff,
			    unsigned long length)
{
	struct spi_buf tx_buf = { .buf = pSendBuffer, .len = pSendBuffer ? length : 0 };
	struct spi_buf rx_buf = { .buf = pRecvBuff,   .len = pRecvBuff  ? length : 0 };

	const struct spi_buf_set tx_set = { .buffers = &tx_buf, .count = 1 };
	const struct spi_buf_set rx_set = { .buffers = &rx_buf, .count = 1 };

	int ret = spi_transceive(spi_dev, &spi_cfg, &tx_set, &rx_set);
	if (ret < 0) {
		LOG_ERR("SPI transceive error: %d", ret);
	}
}

void AD5940_CsClr(void)
{
	gpio_pin_set_dt(&cs_gpio, 1);   /* active-low: logical 1 → pin low */
}

void AD5940_CsSet(void)
{
	gpio_pin_set_dt(&cs_gpio, 0);   /* active-low: logical 0 → pin high */
}

void AD5940_RstSet(void)
{
	gpio_pin_set_dt(&rst_gpio, 0);  /* de-assert reset (pin high) */
}

void AD5940_RstClr(void)
{
	gpio_pin_set_dt(&rst_gpio, 1);  /* assert reset (pin low) */
}

void AD5940_Delay10us(uint32_t time)
{
	time *= 10;
	if (time == 0) {
		time = 1;
	}
	k_busy_wait(time);
}

uint32_t AD5940_GetMCUIntFlag(void)
{
	return ucInterrupted;
}

uint32_t AD5940_ClrMCUIntFlag(void)
{
	ucInterrupted = 0;
	return 1;
}

/* ---------- Resource init / deinit ---------- */

uint32_t AD5940_MCUResourceInit(void)
{
	int ret;

	/* Verify SPI device */
	if (!device_is_ready(spi_dev)) {
		LOG_ERR("SPI device not ready");
		return 1;
	}
	LOG_INF("SPI device ready");

	/* CS pin — output, start de-asserted (high) */
	ret = gpio_pin_configure_dt(&cs_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("CS gpio config failed: %d", ret);
		return 1;
	}

	/* RST pin — output, start de-asserted (high) */
	ret = gpio_pin_configure_dt(&rst_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("RST gpio config failed: %d", ret);
		return 1;
	}

	/* GP0INT pin — input, falling-edge interrupt */
	ret = gpio_pin_configure_dt(&int_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("INT gpio config failed: %d", ret);
		return 1;
	}
	ret = gpio_pin_interrupt_configure_dt(&int_gpio, GPIO_INT_EDGE_TO_INACTIVE);
	if (ret < 0) {
		LOG_ERR("INT interrupt config failed: %d", ret);
		return 1;
	}
	gpio_init_callback(&int_cb_data, gp0int_isr, BIT(int_gpio.pin));
	gpio_add_callback(int_gpio.port, &int_cb_data);

	LOG_INF("AD5940 MCU resources initialized (CS, RST, INT)");
	return 0;
}
