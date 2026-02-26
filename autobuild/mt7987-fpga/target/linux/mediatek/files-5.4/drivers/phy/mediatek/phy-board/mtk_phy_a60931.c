// SPDX-License-Identifier: GPL-2.0
/*
 * MediaTek PCIe phy board controller driver.
 *
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Chuanjia Liu <chuanjia.liu@mediatek.com>
 */

#include <linux/kernel.h>

#include "mtk_gpio_i2c.h"

void mtk_phy_switch_lane(unsigned int lane)
{
	unsigned char tmp;
	int i;

	if (lane == 1)
		tmp = 0x1;
	else
		tmp = 0;

	/* Change bank address to 0x70 */
	mtk_i2c_write_field(0x70, 0xfc, 31, 24, 0x70);

	mtk_i2c_write_field(0x70, 0x4, 4, 4, tmp);

	for (i = 10; i < 22; i++)
		mtk_i2c_write_field(0x70, 0x4, i, i, tmp);
}

void mtk_phy_base_init(unsigned int lane)
{
	/* A60931(4 Lane PCIe) Bank Selection from I2C
	 *****************************************************************
	 *PCIe I2C Device Addr : 0x70/0x71/0x72/0x73 for Lane0/1/2/3
	 *Bank 1: PHYD
	 *Bank 2: PHYD2
	 *Bank 3: PHYA
	 *Bank 4: PHYA_DA
	 *Bank 6: SPLL
	 *Bank 7: GLB
	 *****************************************************************
	 */

	/* Change bank address to 0x20 */
	mtk_i2c_write_field(0x70, 0xfc, 31, 24, 0x20);

	/* rg_ssusb_force_pcie_mode_sel=b0, Host mode */
	mtk_i2c_write_field(0x70, 0x0, 4, 4, 0x0);

	/* rg_ssusb_force_pcie_mode_en=b1 */
	mtk_i2c_write_field(0x70, 0x0, 3, 3, 0x1);

	/* rg_ssusb_force_phy_mode=0b0, PCI-e mode */
	mtk_i2c_write_field(0x70, 0x0, 2, 1, 0x0);

	/* rg_ssusb_force_phy_mode_en=0b1 */
	mtk_i2c_write_field(0x70, 0x0, 0, 0, 0x1);

	/* Change bank address to 0x30 */
	mtk_i2c_write_field(0x70, 0xfc, 31, 24, 0x30);

	/* SSUSB 1.0V power ON(LN0) */
	mtk_i2c_write_field(0x70, 0x4, 29, 29, 0x1);

	/* Change bank address to 0x30 */
	mtk_i2c_write_field(0x71, 0xfc, 31, 24, 0x30);

	/* SSUSB 1.0V power ON(LN1) */
	mtk_i2c_write_field(0x71, 0x4, 29, 29, 0x1);

	/* Change bank address to 0x30 */
	mtk_i2c_write_field(0x72, 0xfc, 31, 24, 0x30);

	/* SSUSB 1.0V power ON(LN2) */
	mtk_i2c_write_field(0x72, 0x4, 29, 29, 0x1);

	/* Change bank address to 0x30 */
	mtk_i2c_write_field(0x73, 0xfc, 31, 24, 0x30);

	/* SSUSB 1.0V power ON(LN3) */
	mtk_i2c_write_field(0x73, 0x4, 29, 29, 0x1);

	/* Change bank address to 0x70 */
	mtk_i2c_write_field(0x70, 0xfc, 31, 24, 0x70);

	/* Pipe reset, clk driving current */
	mtk_i2c_write_field(0x70, 0x88, 3, 2, 0x2);

	/* Data lane 0 driving current */
	mtk_i2c_write_field(0x70, 0x88, 5, 4, 0x2);

	/* Data lane 1 driving current */
	mtk_i2c_write_field(0x70, 0x88, 7, 6, 0x2);

	/* Data lane 2 driving current */
	mtk_i2c_write_field(0x70, 0x88, 9, 8, 0x2);

	/* Data lane 3 driving current */
	mtk_i2c_write_field(0x70, 0x88, 11, 10, 0x2);

	/* PCLK phase 0x00~0x1F  (0x1B) */

	/*
	 **********************************
	 *		PCI-e PLL BW
	 **********************************
	 */

	/* Change bank address to 0x40 */
	mtk_i2c_write_field(0x70, 0xfc, 31, 24, 0x40);

	/* RG_SSUSB_PLL_IC_PE2D */
	mtk_i2c_write_field(0x70, 0xc, 19, 16, 0x8);

	/* RG_SSUSB_PLL_IR_PE2D */
	mtk_i2c_write_field(0x70, 0x10, 27, 24, 0xB);

	/* RG_SSUSB_PLL_BR_PE2D */
	mtk_i2c_write_field(0x70, 0xc, 31, 30, 0x1);

	/* Change bank address to 0x40 */
	mtk_i2c_write_field(0x71, 0xfc, 31, 24, 0x40);

	/* RG_SSUSB_PLL_IC_PE2D */
	mtk_i2c_write_field(0x71, 0xc, 19, 16, 0x8);

	/* RG_SSUSB_PLL_IR_PE2D */
	mtk_i2c_write_field(0x71, 0x10, 27, 24, 0xB);

	/* RG_SSUSB_PLL_BR_PE2D */
	mtk_i2c_write_field(0x71, 0xc, 31, 30, 0x1);

	/* Change bank address to 0x40 */
	mtk_i2c_write_field(0x72, 0xfc, 31, 24, 0x40);

	/* RG_SSUSB_PLL_IC_PE2D */
	mtk_i2c_write_field(0x72, 0xc, 19, 16, 0x8);

	/* RG_SSUSB_PLL_IR_PE2D */
	mtk_i2c_write_field(0x72, 0x10, 27, 24, 0xB);

	/* RG_SSUSB_PLL_BR_PE2D */
	mtk_i2c_write_field(0x72, 0xc, 31, 30, 0x1);

	/* Change bank address to 0x40 */
	mtk_i2c_write_field(0x73, 0xfc, 31, 24, 0x40);

	/* RG_SSUSB_PLL_IC_PE2D */
	mtk_i2c_write_field(0x73, 0xc, 19, 16, 0x8);

	/* RG_SSUSB_PLL_IR_PE2D */
	mtk_i2c_write_field(0x73, 0x10, 27, 24, 0xB);

	/* RG_SSUSB_PLL_BR_PE2D */
	mtk_i2c_write_field(0x73, 0xc, 31, 30, 0x1);

	/*
	 **********************************
	 *		   EQ_Setting
	 **********************************
	 */

	/* Change bank address to 0x10 */
	mtk_i2c_write_field(0x70, 0xfc, 31, 24, 0x10);

	/* EQ_DLEQ_LFI_GEN2 */
	mtk_i2c_write_field(0x70, 0x7c, 31, 28, 0x3);

	/* EQ_DLEQ_LFI_GEN1 */
	mtk_i2c_write_field(0x70, 0x7c, 27, 24, 0x3);

	/* Change bank address to 0x10 */
	mtk_i2c_write_field(0x71, 0xfc, 31, 24, 0x10);

	/* EQ_DLEQ_LFI_GEN2 */
	mtk_i2c_write_field(0x71, 0x7c, 31, 28, 0x3);

	/* EQ_DLEQ_LFI_GEN1 */
	mtk_i2c_write_field(0x71, 0x7c, 27, 24, 0x3);

	/* Change bank address to 0x10 */
	mtk_i2c_write_field(0x72, 0xfc, 31, 24, 0x10);

	/* EQ_DLEQ_LFI_GEN2 */
	mtk_i2c_write_field(0x72, 0x7c, 31, 28, 0x3);

	/* EQ_DLEQ_LFI_GEN1 */
	mtk_i2c_write_field(0x72, 0x7c, 27, 24, 0x3);

	/* Change bank address to 0x10 */
	mtk_i2c_write_field(0x73, 0xfc, 31, 24, 0x10);

	/* EQ_DLEQ_LFI_GEN2 */
	mtk_i2c_write_field(0x73, 0x7c, 31, 28, 0x3);

	/* EQ_DLEQ_LFI_GEN1 */
	mtk_i2c_write_field(0x73, 0x7c, 27, 24, 0x3);

	/*
	 **********************************
	 *       BBPD LDO Turn OFF
	 **********************************
	 */

	/* Change bank address to 0x30 */
	mtk_i2c_write_field(0x70, 0xfc, 31, 24, 0x30);

	/* RG_SSUSB_LN0_CDR_RESERVE[0] */
	mtk_i2c_write_field(0x70, 0x18, 24, 24, 0x1);

	/* Change bank address to 0x30 */
	mtk_i2c_write_field(0x71, 0xfc, 31, 24, 0x30);

	/* RG_SSUSB_LN0_CDR_RESERVE[0] */
	mtk_i2c_write_field(0x71, 0x18, 24, 24, 0x1);

	/* Change bank address to 0x30 */
	mtk_i2c_write_field(0x72, 0xfc, 31, 24, 0x30);

	/* RG_SSUSB_LN0_CDR_RESERVE[0] */
	mtk_i2c_write_field(0x72, 0x18, 24, 24, 0x1);

	/* Change bank address to 0x30 */
	mtk_i2c_write_field(0x73, 0xfc, 31, 24, 0x30);

	/* RG_SSUSB_LN0_CDR_RESERVE[0] */
	mtk_i2c_write_field(0x73, 0x18, 24, 24, 0x1);

	/*
	 **********************************
	 *  Set INTR & TX/RX Impedance - P1
	 **********************************
	 */

	/* Change bank address to 0x30 */
	mtk_i2c_write_field(0x70, 0xfc, 31, 24, 0x30);

	/* INTR_EN */
	mtk_i2c_write_field(0x70, 0x0, 26, 26, 0x1);

	/* Set Iext R selection */
	mtk_i2c_write_field(0x70, 0x0, 15, 10, 0x2f);

	/* Change bank address to 0x10 */
	mtk_i2c_write_field(0x70, 0xfc, 31, 24, 0x10);

	/* Force da_ssusb_tx_impsel enable */
	mtk_i2c_write_field(0x70, 0x10, 31, 31, 0x1);

	/* Set TX Impedance */
	mtk_i2c_write_field(0x70, 0x10, 28, 24, 0x11);

	/* Force da_ssusb_rx_impsel enable*/
	mtk_i2c_write_field(0x70, 0x14, 31, 31, 0x1);

	/* Set RX Impedance */
	mtk_i2c_write_field(0x70, 0x14, 28, 24, 0xf);

	/* Change bank address to 0x10 */
	mtk_i2c_write_field(0x71, 0xfc, 31, 24, 0x10);

	/* Force da_ssusb_tx_impsel enable */
	mtk_i2c_write_field(0x71, 0x10, 31, 31, 0x1);

	/* Set TX Impedance */
	mtk_i2c_write_field(0x71, 0x10, 28, 24, 0x11);

	/* Force da_ssusb_rx_impsel enable*/
	mtk_i2c_write_field(0x71, 0x14, 31, 31, 0x1);

	/* Set RX Impedance */
	mtk_i2c_write_field(0x71, 0x14, 28, 24, 0xf);

	/* Change bank address to 0x10 */
	mtk_i2c_write_field(0x72, 0xfc, 31, 24, 0x10);

	/* Force da_ssusb_tx_impsel enable */
	mtk_i2c_write_field(0x72, 0x10, 31, 31, 0x1);

	/* Set TX Impedance */
	mtk_i2c_write_field(0x72, 0x10, 28, 24, 0x11);

	/* Force da_ssusb_rx_impsel enable*/
	mtk_i2c_write_field(0x72, 0x14, 31, 31, 0x1);

	/* Set RX Impedance */
	mtk_i2c_write_field(0x72, 0x14, 28, 24, 0xf);

	/* Change bank address to 0x10 */
	mtk_i2c_write_field(0x73, 0xfc, 31, 24, 0x10);

	/* Force da_ssusb_tx_impsel enable */
	mtk_i2c_write_field(0x73, 0x10, 31, 31, 0x1);

	/* Set TX Impedance */
	mtk_i2c_write_field(0x73, 0x10, 28, 24, 0x11);

	/* Force da_ssusb_rx_impsel enable*/
	mtk_i2c_write_field(0x73, 0x14, 31, 31, 0x1);

	/* Set RX Impedance */
	mtk_i2c_write_field(0x73, 0x14, 28, 24, 0xf);

	/*
	 * In this part ,different lane has different setting,
	 * 1lane need disable other lane.
	 */
	mtk_phy_switch_lane(lane);

	/*
	 *****************************************************************
	 * PCI-e Host Patch: set PLL reference clock input path, 2015/3/24
	 *****************************************************************
	 */
	/* Change bank address to 0x40 */
	mtk_i2c_write_field(0x70, 0xfc, 31, 24, 0x40);

	/* RG_SSUSB_XTAL_EXT_EN_PE2H */
	mtk_i2c_write_field(0x70, 0x0, 17, 16, 0x2);

	/* RG_SSUSB_XTAL_EXT_EN_PE1H */
	mtk_i2c_write_field(0x70, 0x0, 13, 12, 0x2);

	/* Change bank address to 0x10 */
	mtk_i2c_write_field(0x70, 0xfc, 31, 24, 0x10);

	/* force_pll_sscen */
	mtk_i2c_write_field(0x70, 0x38, 15, 15, 0x1);

	/* pll_sscen */
	mtk_i2c_write_field(0x70, 0x38, 14, 14, 0x1);

	/* Change bank address to 0x70 */
	mtk_i2c_write_field(0x70, 0xfc, 31, 24, 0x70);

	mtk_i2c_write_field(0x70, 0x7, 7, 3, 0x6);

	/* Probe */
	/* Change bank address to 0x10 */
	mtk_i2c_write_field(0x70, 0xfc, 31, 24, 0x10);

	mtk_i2c_write_field(0x70, 0x52, 5, 0, 0x3d);

	/*
	 *****************************************************************
	 *   PCIe 2lane patch ,this setting not affect 1 lane
	 *****************************************************************
	 */

	/* Change bank address to 0x70 */
	mtk_i2c_write_field(0x70, 0xfc, 31, 24, 0x70);
	//Force ln2 on
	mtk_i2c_write_field(0x70, 0x8, 4, 4, 0x1);
	//Force ln3 on
	mtk_i2c_write_field(0x70, 0x8, 6, 6, 0x1);

	/* Change bank address to 0x70 */
	mtk_i2c_write_field(0x70, 0xfc, 31, 24, 0x70);

	/*
	 *****************************************************************
	 *   PCIe Host P2 patch: Not gate TX/PCLK clock at P2  2021/12/15
	 *****************************************************************
	 */

	/* Change bank address to 0x10 */
	mtk_i2c_write_field(0x70, 0xfc, 31, 24, 0x10);

	mtk_i2c_write_field(0x70, 0x4, 21, 16, 0x33);

	/* Change bank address to 0x10 */
	mtk_i2c_write_field(0x70, 0xfc, 31, 24, 0x10);

	/*
	 * rg_ssusb_p_p3_tx_ng ; PCIe Host mode only,
	 * 1'b0: Gate TX clock at P2, 1'b1: Not gate TX clock at P2
	 */
	mtk_i2c_write_field(0x70, 0x0, 31, 31, 0x1);

	/* rg_ssusb_p_p3_pclk_ng; PCIe Host mode only,
	 * 1'b0: Gate PCLK clock at P2 , 1'b1: Not gate PCLK clock at P2
	 */
	mtk_i2c_write_field(0x70, 0x0, 27, 27, 0x1);

	pr_info("=== %s done! ===\n", __func__);
}

unsigned char mtk_phy_read_phase(void)
{
	unsigned char val = 0;

	/* Change bank */
	mtk_i2c_write_field(0x70, 0xfc, 31, 24, 0x70);
	/* Read phase */
	mtk_i2c_read_field(0x70, 0x9c, 4, 0, &val);

	return val;
}

void mtk_phy_write_phase(unsigned char phase)
{
	/* Change bank */
	mtk_i2c_write_field(0x70, 0xfc, 31, 24, 0x70);
	/* Write phase */
	mtk_i2c_write_field(0x70, 0x9c, 4, 0, phase);
}

int mtk_phy_set_port(const char *full_name)
{
	return mtk_gpio_set_port(full_name);
}

int mtk_phy_init(unsigned int addr, unsigned int dir_offset,
			unsigned int dout_offset, unsigned int din_offset,
			unsigned int sda, unsigned int scl, unsigned int lane, const char *full_name)
{
	int ret = 0;

	ret = mtk_gpio_init(addr, dir_offset, dout_offset,
					din_offset, sda, scl, full_name);
	if (ret)
		return ret;

	mtk_phy_base_init(lane);

	return 0;
}

int mtk_phy_exit(const char *full_name)
{
	return mtk_gpio_exit(full_name);
}

