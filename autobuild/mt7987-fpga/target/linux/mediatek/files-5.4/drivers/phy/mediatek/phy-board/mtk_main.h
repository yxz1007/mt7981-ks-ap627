/* SPDX-License-Identifier: GPL-2.0 */
/*
 * MediaTek PCIe phy board controller driver.
 *
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Chuanjia Liu <chuanjia.liu@mediatek.com>
 */

#ifndef __MTK_COMMON_H__
#define __MTK_COMMON_H__

#include <linux/io.h>

#include "mtk_pcie_gen3.h"
#include "mtk_phy_a60931.h"

#define AUTO_SCAN	0xFF

enum pcie_lane {
	ONE = 1,
	TWO,
	FOUR = 4,
};

enum pcie_speed {
	GEN1 = 1,
	GEN2,
	GEN3,
	GEN4,
};

struct cmd_tbl {
	char *name;
	int (*cb_func)(int argc, char **argv);
	char *help;
};

struct mtk_emul_info {
	const char *full_name;
	unsigned int gpio_addr;
	unsigned int gpio_dir_offset;
	unsigned int gpio_dout_offset;
	unsigned int gpio_din_offset;
	unsigned int pin_sda;
	unsigned int pin_scl;
	unsigned int pcie_addr;
	enum pcie_speed speed;
	enum pcie_lane lane;
	unsigned int phase;
	void __iomem *port_base;
	struct kobject *main_kobj;
};

struct mtk_emul_info *mtk_ctrl_info;
struct mtk_emul_info *mtk_ctrl[10];

#endif
