/* SPDX-License-Identifier: GPL-2.0 */
/*
 * MediaTek PCIe phy board controller driver.
 *
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Chuanjia Liu <chuanjia.liu@mediatek.com>
 */

#ifndef __MTK_GPIO_I2C_H__
#define __MTK_GPIO_I2C_H__

/* GPIO DIR: 0 as input, 1 as output */
enum gpio_dir {
	GPIO_INPUT,
	GPIO_OUTPUT,
};

/* GPIO output: 0 as output low, 1 as output high */
enum gpio_out_level {
	OUTPUT_LOW,
	OUTPUT_HIGH,
};

struct mtk_gpio_info {
	const char *full_name;
	void __iomem *base;
	void __iomem *dir;
	void __iomem *dout;
	void __iomem *din;
	unsigned int sda;
	unsigned int scl;
};

int mtk_gpio_init(unsigned int addr, unsigned int dir_offset,
			unsigned int dout_offset, unsigned int din_offset,
			unsigned int sda, unsigned int scl, const char *full_name);
int mtk_gpio_exit(const char *full_name);
int mtk_gpio_set_port(const char *full_name);
void mtk_i2c_write_field(unsigned char dev_id, unsigned char addr,
				int high, int low, unsigned char data);
void mtk_i2c_read_field(unsigned char dev_id, unsigned char addr,
				int high, int low, unsigned char *data);

#endif
