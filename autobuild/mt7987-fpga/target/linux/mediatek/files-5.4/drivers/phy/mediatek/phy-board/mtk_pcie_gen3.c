// SPDX-License-Identifier: GPL-2.0
/*
 * MediaTek PCIe phy board controller driver.
 *
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Chuanjia Liu <chuanjia.liu@mediatek.com>
 */

#include <linux/iopoll.h>

#include "mtk_pcie_gen3.h"

void mtk_pcie_set_max_speed(void __iomem *port_base, unsigned int speed)
{
	int val;

	val = readl_relaxed(port_base + PCIE_SETTING_REG);

	val &= ~GENMASK(14, 12);
	switch (speed) {
	case 4:
		val |= PCIE_GEN4_SUPPORT;
	case 3:
		val |= PCIE_GEN3_SUPPORT;
	case 2:
		val |= PCIE_GEN2_SUPPORT;
	default:
		break;
	}

	writel_relaxed(val, port_base + PCIE_SETTING_REG);
}

void mtk_pcie_set_max_lane(void __iomem *port_base, unsigned int lane)
{
	int val;

	val = readl_relaxed(port_base + PCIE_SETTING_REG);

	val &= ~GENMASK(11, 8);
	switch (lane) {
	case 16:
		val |= PCIE_16LANE_SUPPORT;
	case 8:
		val |= PCIE_8LANE_SUPPORT;
	case 4:
		val |= PCIE_4LANE_SUPPORT;
	case 2:
		val |= PCIE_2LANE_SUPPORT;
	default:
		break;
	}

	writel_relaxed(val, port_base + PCIE_SETTING_REG);
}

void mtk_pcie_init(void __iomem *port_base, unsigned int speed,
				unsigned int lane)
{
	int val;

	/* Set as RC mode */
	val = readl_relaxed(port_base + PCIE_SETTING_REG);
	val |= PCIE_RC_MODE;

	writel_relaxed(val, port_base + PCIE_SETTING_REG);

	mtk_pcie_set_max_speed(port_base, speed);

	mtk_pcie_set_max_lane(port_base, lane);

	mtk_pcie_assert_reset(port_base);

	pr_info("=== %s done! ===\n", __func__);
}

void mtk_pcie_get_current_info(void __iomem *port_base,
				unsigned int *speed, unsigned int *lane)
{
	int val;

	val = readl_relaxed(port_base + PCIE_BASIC_STATUS);
	*speed = (val & GENMASK(11, 8)) >> 8;
	*lane = val & GENMASK(4, 0);
}

void mtk_pcie_assert_reset(void __iomem *port_base)
{
	int val;

	/* assert readl_relaxed reset signals */
	val = readl_relaxed(port_base + PCIE_RST_CTRL_V3);
	val |= PCIE_MAC_RSTB_V3 | PCIE_PHY_RSTB_V3 | PCIE_BRG_RSTB_V3|
	       PCIE_PE_RSTB_V3;
	writel_relaxed(val, port_base + PCIE_RST_CTRL_V3);
}

int mtk_pcie_relink(void __iomem *port_base)
{
	int val;

	/* assert readl_relaxed reset signals */
	val = readl_relaxed(port_base + PCIE_RST_CTRL_V3);
	val |= PCIE_MAC_RSTB_V3 | PCIE_PHY_RSTB_V3 | PCIE_BRG_RSTB_V3|
	       PCIE_PE_RSTB_V3;
	writel_relaxed(val, port_base + PCIE_RST_CTRL_V3);

	/* de-assert reset signals*/
	val &= ~(PCIE_MAC_RSTB_V3 | PCIE_PHY_RSTB_V3 | PCIE_BRG_RSTB_V3);
	writel_relaxed(val, port_base + PCIE_RST_CTRL_V3);

	usleep_range(100 * 1000, 120 * 1000);

	/* de-assert pe reset signals*/
	val &= ~PCIE_PE_RSTB_V3;
	writel_relaxed(val, port_base + PCIE_RST_CTRL_V3);

	/* 100ms timeout value should be enough for Gen1/2 training */
	return readl_poll_timeout(port_base + PCIE_DATA_LINK_STATUS_V3, val,
			!!(val & PCIE_DATA_LINKUP_V3), 20,
			200 * USEC_PER_MSEC);
}

