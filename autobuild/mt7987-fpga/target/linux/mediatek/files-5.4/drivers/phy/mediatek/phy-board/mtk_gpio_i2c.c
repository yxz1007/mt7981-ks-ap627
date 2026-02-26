// SPDX-License-Identifier: GPL-2.0
/*
 * MediaTek PCIe phy board controller driver.
 *
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Chuanjia Liu <chuanjia.liu@mediatek.com>
 */

#include <linux/io.h>
#include <linux/string.h>

#include "mtk_gpio_i2c.h"

static struct mtk_gpio_info gpio_infos[10];
static struct mtk_gpio_info *gpio_info;
static unsigned int count;

#define I2C_DELAY 100
#define mtk_i2c_delay(delay) for (count = (delay); count != 0; count--)

static void mtk_gpio_set_dir(unsigned int dir, unsigned int pin)
{
	struct mtk_gpio_info *gpio = gpio_info;
	unsigned int val, offset;

	offset = pin & 0x1f;

	val = readl(gpio->dir);

	if (dir == GPIO_OUTPUT)
		writel(val | (1 << offset), gpio->dir);
	else
		writel(val & ~(1 << offset), gpio->dir);
}

static void mtk_gpio_set_level(unsigned int data, unsigned int pin)
{
	struct mtk_gpio_info *gpio = gpio_info;
	unsigned int val, offset;

	offset = pin & 0x1f;

	val = readl(gpio->dout);

	if (data == OUTPUT_HIGH)
		writel(val | (1 << offset), gpio->dout);
	else
		writel(val & ~(1 << offset), gpio->dout);
}

static unsigned int mtk_gpio_get_din(unsigned int pin)
{
	struct mtk_gpio_info *gpio = gpio_info;
	unsigned int offset, data;

	offset = pin & 0x1f;

	data = readl(gpio->din);
	data = data & (1 << offset);
	data = (data >> offset);

	return (unsigned int)data;
}

int mtk_gpio_set_port(const char *full_name)
{
	int i;

	for (i = 0; i < 10; i++) {
		if ((gpio_infos[i].full_name != NULL) && !strcmp(gpio_infos[i].full_name, full_name)) {
			gpio_info = &gpio_infos[i];
			break;
		}
	}
	if (i == 10)
		return -EINVAL;

	return 0;
}

int mtk_gpio_init(unsigned int addr, unsigned int dir_offset,
			unsigned int dout_offset, unsigned int din_offset,
			unsigned int sda, unsigned int scl, const char *full_name)
{
	struct mtk_gpio_info *gpio;
	int i;

	for (i = 0; i < 10; i++) {
		if ((gpio_infos[i].full_name != NULL) && !strcmp(gpio_infos[i].full_name, full_name)) {
			gpio_info = &gpio_infos[i];
			break;
		}
		if (gpio_infos[i].full_name == NULL) {
			gpio_info = &gpio_infos[i];
			break;
		}
	}
	if (i == 10)
		return -EINVAL;

	gpio = gpio_info;
	gpio->base = ioremap(addr, 0x1000);
	if (!gpio->base) {
		pr_err("mapping gpio addr failed\n");
		return -ENOMEM;
	}

	gpio->dir = gpio->base + dir_offset;
	gpio->dout = gpio->base + dout_offset;
	gpio->din = gpio->base + din_offset;
	gpio->scl = scl;
	gpio->sda = sda;
	gpio->full_name = full_name;

	pr_info("=== %s done! ===\n", __func__);

	return 0;
}

int mtk_gpio_exit(const char *full_name)
{
	struct mtk_gpio_info *gpio;
	int i;

	for (i = 0; i < 10; i++) {
		if ((gpio_infos[i].full_name != NULL) && !strcmp(gpio_infos[i].full_name, full_name)) {
			gpio_info = &gpio_infos[i];
			break;
		}
	}
	if (i == 10)
		return -EINVAL;

	gpio = gpio_info;
	iounmap(gpio->base);

	gpio->base = NULL;
	gpio->dir = NULL;
	gpio->dout = NULL;
	gpio->din = NULL;
	gpio->scl = 0;
	gpio->sda = 0;
	gpio->full_name = NULL;

	pr_info("=== %s done! ===\n", __func__);
	return 0;
}

/* Start sequence of I2C
 * Prepare the SDA and SCL for sending/receiving
 * SCL keep high, SDA high -> low
 */
static void mtk_i2c_start(void)
{
	mtk_gpio_set_dir(GPIO_OUTPUT, gpio_info->sda);
	mtk_gpio_set_dir(GPIO_OUTPUT, gpio_info->scl);

	mtk_gpio_set_level(OUTPUT_HIGH, gpio_info->sda);
	mtk_gpio_set_level(OUTPUT_HIGH, gpio_info->scl);
	mtk_i2c_delay(I2C_DELAY);

	mtk_gpio_set_level(OUTPUT_LOW, gpio_info->sda);
	mtk_i2c_delay(I2C_DELAY);

	mtk_gpio_set_level(OUTPUT_LOW, gpio_info->scl);
	mtk_i2c_delay(I2C_DELAY);
}

/* Stop sequence of I2C
 * Prepare the SDA and SCL for stop
 * SCL keep high, SDA low -> high
 */
static void mtk_i2c_stop(void)
{
	mtk_gpio_set_dir(GPIO_OUTPUT, gpio_info->sda);

	mtk_gpio_set_level(OUTPUT_LOW, gpio_info->scl);
	mtk_gpio_set_level(OUTPUT_LOW, gpio_info->sda);
	mtk_i2c_delay(I2C_DELAY);

	mtk_gpio_set_level(OUTPUT_HIGH, gpio_info->scl);
	mtk_i2c_delay(I2C_DELAY);

	mtk_gpio_set_level(OUTPUT_HIGH, gpio_info->sda);
	mtk_i2c_delay(I2C_DELAY);

	mtk_gpio_set_dir(GPIO_INPUT, gpio_info->sda);
	mtk_gpio_set_dir(GPIO_INPUT, gpio_info->scl);
}

/* ack -> sda low , nak -> sda high*/
static void mtk_i2c_send_ack(int ack)
{
	mtk_gpio_set_dir(GPIO_OUTPUT, gpio_info->sda);
	mtk_gpio_set_level(ack, gpio_info->sda);
	mtk_i2c_delay(I2C_DELAY);

	mtk_gpio_set_level(OUTPUT_HIGH, gpio_info->scl);
	mtk_i2c_delay(I2C_DELAY);
	mtk_gpio_set_level(OUTPUT_LOW, gpio_info->scl);
	mtk_i2c_delay(I2C_DELAY);
}

/* ack is 0 ,nak is 1 */
static int mtk_i2c_get_ack(void)
{
	int ack;

	mtk_gpio_set_level(OUTPUT_LOW, gpio_info->sda);
	mtk_i2c_delay(I2C_DELAY);
	mtk_gpio_set_dir(GPIO_INPUT, gpio_info->sda);
	mtk_i2c_delay(I2C_DELAY);
	mtk_gpio_set_level(OUTPUT_HIGH, gpio_info->scl);
	mtk_i2c_delay(I2C_DELAY);

	ack = mtk_gpio_get_din(gpio_info->sda);
	mtk_gpio_set_level(OUTPUT_LOW, gpio_info->scl);
	mtk_i2c_delay(I2C_DELAY);

	return ack;
}

static void mtk_i2c_write_byte(unsigned char data)
{
	int i;

	mtk_gpio_set_dir(GPIO_OUTPUT, gpio_info->sda);

	for (i = 8; --i >= 0;) {
		mtk_gpio_set_level((data >> i) & 0x01, gpio_info->sda);
		mtk_i2c_delay(I2C_DELAY);
		mtk_gpio_set_level(OUTPUT_HIGH, gpio_info->scl);
		mtk_i2c_delay(I2C_DELAY);
		mtk_gpio_set_level(OUTPUT_LOW, gpio_info->scl);
		mtk_i2c_delay(I2C_DELAY);
	}
}

static void mtk_i2c_read_byte(unsigned char *data)
{
	int i;
	unsigned char tmp = 0;

	mtk_gpio_set_dir(GPIO_INPUT, gpio_info->sda);

	for (i = 8; --i >= 0;) {
		tmp <<= 1;
		mtk_gpio_set_level(OUTPUT_HIGH, gpio_info->scl);
		mtk_i2c_delay(I2C_DELAY);
		tmp |= mtk_gpio_get_din(gpio_info->sda);
		mtk_gpio_set_level(OUTPUT_LOW, gpio_info->scl);
		mtk_i2c_delay(I2C_DELAY);
	}

	*data = tmp;
}

static int mtk_i2c_write(unsigned char dev_id, unsigned char addr,
				unsigned char data)
{
	mtk_i2c_start();
	/* write cmd */
	mtk_i2c_write_byte((dev_id << 1) & 0xff);
	if (mtk_i2c_get_ack())
		goto __err;

	mtk_i2c_write_byte(addr);
	if (mtk_i2c_get_ack())
		goto __err;

	mtk_i2c_write_byte(data);
	if (mtk_i2c_get_ack())
		goto __err;

	mtk_i2c_stop();
	return 0;

__err:
	mtk_i2c_stop();
	pr_info("=== %s fail! ===\n", __func__);
	return -1;
}

static int mtk_i2c_read(unsigned char dev_id, unsigned char addr,
				unsigned char *data)
{
	mtk_i2c_start();

	/* write cmd */
	mtk_i2c_write_byte((dev_id << 1) & 0xff);
	if (mtk_i2c_get_ack())
		goto __err;

	mtk_i2c_write_byte(addr);
	if (mtk_i2c_get_ack())
		goto __err;

	mtk_i2c_start();
	/* read cmd */
	mtk_i2c_write_byte(((dev_id << 1) & 0xff) | 0x01);
	if (mtk_i2c_get_ack())
		goto __err;

	mtk_i2c_read_byte(data);
	/* every read byte need send ack, but last need nak */
	mtk_i2c_send_ack(1);

	mtk_i2c_stop();
	return 0;

__err:
	mtk_i2c_stop();
	pr_info("=== %s fail! ===\n", __func__);
	return -1;
}

/* phy script e.g  "I2C  0x70  0xFC[31:24]  0x20   RW"
 *  mtk_i2c_write_field(0x70, 0xFC, 31, 24, 0x20)
 */
void mtk_i2c_write_field(unsigned char dev_id, unsigned char addr,
			int high, int low, unsigned char data)
{
	unsigned char tmp = high / 8;

	if (tmp > 0) {
		addr = addr + tmp;
		high = high - (tmp * 8);
		low = low - (tmp * 8);
	}

	mtk_i2c_read(dev_id, addr, &tmp);
	tmp &= ~GENMASK(high, low);
	tmp |= (data << low) & GENMASK(high, low);
	mtk_i2c_write(dev_id, addr, tmp);
}

void mtk_i2c_read_field(unsigned char dev_id, unsigned char addr,
				int high, int low, unsigned char *data)
{
	unsigned char tmp = high / 8;

	if (tmp > 0) {
		addr = addr + tmp;
		high = high - (tmp * 8);
		low = low - (tmp * 8);
	}

	mtk_i2c_read(dev_id, addr, &tmp);
	tmp &= GENMASK(high, low);
	*data = tmp;
}

