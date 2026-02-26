/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: Chunfeng Yun <chunfeng.yun@mediatek.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/delay.h>

#define PHY_TRUE 1
#define PHY_FALSE 0

#define SDA 0
#define SCL 1

#define INPUT 0
#define OUTPUT 1

#define SSUSB_FPGA_I2C_OUT_OFFSET 0
#define SSUSB_FPGA_I2C_IN_OFFSET  0x04

#define SSUSB_FPGA_I2C_SDA_OUT (1<<0)
#define SSUSB_FPGA_I2C_SDA_OEN (1<<1)
#define SSUSB_FPGA_I2C_SCL_OUT (1<<2)
#define SSUSB_FPGA_I2C_SCL_OEN (1<<3)

#define SSUSB_FPGA_I2C_SDA_IN_OFFSET 0
#define SSUSB_FPGA_I2C_SCL_IN_OFFSET 1

#define I2C_DELAY 1

#ifdef NEVER_RUN
static void i2c_dummy_delay(u32 count)
{
	do {
		count--;
	} while (count > 0);
}
#else
#define i2c_dummy_delay(count)    udelay(count)
#endif

static
void gpio_set_direction(void *ioi2c, u8 gpio_dir, u8 gpio_pin)
{
	u32 temp;
	void *addr;

	addr = ioi2c + SSUSB_FPGA_I2C_OUT_OFFSET;
	temp = readl(addr);

	if (gpio_pin == SDA) {
		if (gpio_dir == OUTPUT) {
			temp |= SSUSB_FPGA_I2C_SDA_OEN;
			writel(temp, addr);
		} else {
			temp &= ~SSUSB_FPGA_I2C_SDA_OEN;
			writel(temp, addr);
		}
	} else {
		if (gpio_dir == OUTPUT) {
			temp |= SSUSB_FPGA_I2C_SCL_OEN;
			writel(temp, addr);
		} else {
			temp &= ~SSUSB_FPGA_I2C_SCL_OEN;
			writel(temp, addr);
		}
	}
}

static
void gpio_set_value(void *ioi2c, u8 value, u8 gpio_pin)
{
	u32 temp;
	void *addr;

	addr = ioi2c + SSUSB_FPGA_I2C_OUT_OFFSET;
	temp = readl(addr);

	if (gpio_pin == SDA) {
		if (value == 1) {
			temp |= SSUSB_FPGA_I2C_SDA_OUT;
			writel(temp, addr);
		} else {
			temp &= ~SSUSB_FPGA_I2C_SDA_OUT;
			writel(temp, addr);
		}
	} else {
		if (value == 1) {
			temp |= SSUSB_FPGA_I2C_SCL_OUT;
			writel(temp, addr);
		} else {
			temp &= ~SSUSB_FPGA_I2C_SCL_OUT;
			writel(temp, addr);
		}
	}
}

static
u8 gpio_get_value(void *ioi2c, u8 gpio_pin)
{
	u8 temp;
	void *addr;

	addr = ioi2c + SSUSB_FPGA_I2C_IN_OFFSET;
	temp = readl(addr);

	if (gpio_pin == SDA)
		temp = (temp >> SSUSB_FPGA_I2C_SDA_IN_OFFSET) & 0x01;
	else
		temp = (temp >> SSUSB_FPGA_I2C_SCL_IN_OFFSET) & 0x01;

	return temp;
}

static
void i2c_stop(void *ioi2c)
{
	gpio_set_direction(ioi2c, OUTPUT, SDA);
	gpio_set_value(ioi2c, 0, SCL);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(ioi2c, 0, SDA);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(ioi2c, 1, SCL);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(ioi2c, 1, SDA);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_direction(ioi2c, INPUT, SCL);
	gpio_set_direction(ioi2c, INPUT, SDA);
}

 /* Prepare the SDA and SCL for sending/receiving */
static
void i2c_start(void *ioi2c)
{
	gpio_set_direction(ioi2c, OUTPUT, SCL);
	gpio_set_direction(ioi2c, OUTPUT, SDA);
	gpio_set_value(ioi2c, 1, SDA);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(ioi2c, 1, SCL);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(ioi2c, 0, SDA);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(ioi2c, 0, SCL);
	i2c_dummy_delay(I2C_DELAY);
}

 /* return 0 --> ack */
static
u32 i2c_send_byte(void *ioi2c, u8 data)
{
	int i, ack;

	gpio_set_direction(ioi2c, OUTPUT, SDA);

	for (i = 8; --i > 0;) {
		gpio_set_value(ioi2c, (data>>i)&0x01, SDA);
		i2c_dummy_delay(I2C_DELAY);
		gpio_set_value(ioi2c,  1, SCL); /* high */
		i2c_dummy_delay(I2C_DELAY);
		gpio_set_value(ioi2c,  0, SCL); /* low */
		i2c_dummy_delay(I2C_DELAY);
	}
	gpio_set_value(ioi2c, (data>>i)&0x01, SDA);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(ioi2c,  1, SCL); /* high */
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(ioi2c,  0, SCL); /* low */
	i2c_dummy_delay(I2C_DELAY);

	gpio_set_value(ioi2c, 0, SDA);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_direction(ioi2c, INPUT, SDA);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(ioi2c, 1, SCL);
	i2c_dummy_delay(I2C_DELAY);

	 /* ack 1: error , 0:ok */
	ack = gpio_get_value(ioi2c, SDA);
	gpio_set_value(ioi2c, 0, SCL);
	i2c_dummy_delay(I2C_DELAY);

	if (ack == 1)
		return PHY_FALSE;
	else
		return PHY_TRUE;
}

static
void i2c_receive_byte(void *ioi2c, u8 *data, u8 ack)
{
	int i;
	u32 dataCache;

	dataCache = 0;
	gpio_set_direction(ioi2c, INPUT, SDA);

	for (i = 8; --i >= 0;) {
		dataCache <<= 1;
		i2c_dummy_delay(I2C_DELAY);
		gpio_set_value(ioi2c, 1, SCL);
		i2c_dummy_delay(I2C_DELAY);
		dataCache |= gpio_get_value(ioi2c, SDA);
		gpio_set_value(ioi2c, 0, SCL);
		i2c_dummy_delay(I2C_DELAY);
	}

	gpio_set_direction(ioi2c, OUTPUT, SDA);
	gpio_set_value(ioi2c, ack, SDA);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(ioi2c, 1, SCL);
	i2c_dummy_delay(I2C_DELAY);
	gpio_set_value(ioi2c, 0, SCL);
	i2c_dummy_delay(I2C_DELAY);
	*data = (u8)dataCache;
}

static
int i2c_write_reg(void *ioi2c, u8 dev_id, u8 addr, u8 data)
{
	int acknowledge = 0;

	i2c_start(ioi2c);

	acknowledge = i2c_send_byte(ioi2c, (dev_id<<1) & 0xff);
	if (acknowledge)
		acknowledge = i2c_send_byte(ioi2c, addr);
	else
		return PHY_FALSE;

	acknowledge = i2c_send_byte(ioi2c, data);
	if (acknowledge) {
		i2c_stop(ioi2c);
		return PHY_TRUE;
	} else {
		return PHY_FALSE;
	}
}

static
int i2c_read_reg(void *ioi2c, u8 dev_id, u8 addr, u8 *data)
{
	int acknowledge = 0;

	i2c_start(ioi2c);

	acknowledge = i2c_send_byte(ioi2c, (dev_id<<1) & 0xff);
	if (acknowledge)
		acknowledge = i2c_send_byte(ioi2c, addr);
	else
		return PHY_FALSE;

	i2c_start(ioi2c);

	acknowledge = i2c_send_byte(ioi2c, ((dev_id<<1) & 0xff) | 0x01);
	if (acknowledge)
		/* ack 0: ok , 1 error */
		i2c_receive_byte(ioi2c, data, 1);
	else
		return PHY_FALSE;

	i2c_stop(ioi2c);

	return acknowledge;
}

int u3phy_write_reg(void *ioi2c, u8 dev_id, u8 address, int value)
{
	int ret;

	ret = i2c_write_reg(ioi2c, dev_id, address, value);
	if (ret == PHY_FALSE) {
		pr_err("Write failed(dev_id: %x, addr: 0x%x, val: 0x%x)\n",
		       dev_id, address, value);
		return PHY_FALSE;
	}

	return PHY_TRUE;
}

u8 u3phy_read_reg(void *ioi2c, u8 dev_id,  u8 address)
{
	u8 buf;
	int ret;

	ret = i2c_read_reg(ioi2c, dev_id, address, &buf);
	if (ret == PHY_FALSE) {
		pr_err("Read failed(dev_id: %x, addr: 0x%x)\n",
			dev_id, address);
		return PHY_FALSE;
	}
	ret = buf;
	return ret;
}

int u3phy_write_reg32(void *ioi2c, u8 dev_id, u32 addr, u32 data)
{
	u8 addr8;
	u8 data_0, data_1, data_2, data_3;

	addr8 = addr & 0xff;
	data_0 = data & 0xff;
	data_1 = (data>>8) & 0xff;
	data_2 = (data>>16) & 0xff;
	data_3 = (data>>24) & 0xff;

	u3phy_write_reg(ioi2c, dev_id, addr8, data_0);
	u3phy_write_reg(ioi2c, dev_id, addr8+1, data_1);
	u3phy_write_reg(ioi2c, dev_id, addr8+2, data_2);
	u3phy_write_reg(ioi2c, dev_id, addr8+3, data_3);

	return 0;
}

u32 u3phy_read_reg32(void *ioi2c, u8 dev_id, u32 addr)
{
	u8 addr8;
	u32 data;

	addr8 = addr & 0xff;

	data  = u3phy_read_reg(ioi2c, dev_id, addr8);
	data |= (u3phy_read_reg(ioi2c, dev_id, addr8+1) << 8);
	data |= (u3phy_read_reg(ioi2c, dev_id, addr8+2) << 16);
	data |= (u3phy_read_reg(ioi2c, dev_id, addr8+3) << 24);

	return data;
}


int u3phy_write_reg8(void *ioi2c, u8 dev_id, u32 addr, u8 data)
{
	u8 addr8;

	addr8 = addr & 0xff;
	u3phy_write_reg(ioi2c, dev_id, addr8, data);

	return PHY_TRUE;
}

u8 u3phy_read_reg8(void *ioi2c, u8 dev_id, u32 addr)
{
	u8 addr8;
	u32 data;

	addr8 = addr & 0xff;
	data = u3phy_read_reg(ioi2c, dev_id, addr8);

	return data;
}

u32 u3phy_readlmsk(void *ioi2c, u8 i2c_addr, u32 reg_addr32,
	u32 offset, u32 mask)
{
	u32 v;

	v = (u3phy_read_reg32(ioi2c, i2c_addr, reg_addr32) & mask) >> offset;
	return v;
}

int u3phy_writelmsk(void *ioi2c, u8 i2c_addr, u32 reg_addr32,
	u32 offset, u32 mask, u32 data)
{
	u32 cur_value;
	u32 new_value;

	cur_value = u3phy_read_reg32(ioi2c, i2c_addr, reg_addr32);
	new_value = (cur_value & (~mask)) | ((data << offset) & mask);
	u3phy_write_reg32(ioi2c, i2c_addr, reg_addr32, new_value);

	return 0;
}
