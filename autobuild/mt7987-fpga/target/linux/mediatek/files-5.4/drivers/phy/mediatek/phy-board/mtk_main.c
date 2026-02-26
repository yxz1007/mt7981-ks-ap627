// SPDX-License-Identifier: GPL-2.0
/*
 * MediaTek PCIe phy board controller driver.
 *
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Chuanjia Liu <chuanjia.liu@mediatek.com>
 */

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "mtk_main.h"

static void mtk_phase_scan(void)
{
	int link_status;
	unsigned char start_best, end_best, count_best = 0;
	unsigned char start_tmp, count_tmp = 0;
	unsigned char i, target;
	unsigned int cur_speed, cur_lane;

	for (i = 0; i < 32; i++) {
		/* assert reset before change phase, otherwise maybe error */
		mtk_pcie_assert_reset(mtk_ctrl_info->port_base);
		mtk_phy_write_phase(i);
		if (i != mtk_phy_read_phase())
			pr_info("[Warnning!]read back phase != 0x%x.\n", i);

		link_status = mtk_pcie_relink(mtk_ctrl_info->port_base);
		mtk_pcie_get_current_info(mtk_ctrl_info->port_base,
					&cur_speed, &cur_lane);
		if (link_status == 0 && cur_speed == mtk_ctrl_info->speed &&
			cur_lane == mtk_ctrl_info->lane) {
			pr_info("Clock phase 0x%x link good!\n", i);
			if (count_tmp == 0)
				start_tmp = i;
			count_tmp++;
		} else {
			pr_info("Clock phase 0x%x link bad!\n", i);
			pr_info("Bad Reason: cur_speed is %x, cur_lane is %x\n",
						cur_speed, cur_lane);
			if (count_tmp > 0 && count_tmp > count_best) {
				start_best = start_tmp;
				end_best = i - 1;
				count_best = count_tmp;
			}
			count_tmp = 0;
		}
	}

	if (count_tmp > 0 && count_tmp > count_best) {
		start_best = start_tmp;
		end_best = 31;
		count_best = count_tmp;
	}

	if (count_best <= 0) {
		pr_info("No clock phase could be link !\n");
		return;
	}

	target = start_best + (count_best / 2);
	pr_info("Good Regin: start = 0x%x, end = 0x%x => best phase = %x\n",
			start_best, end_best, target);

	mtk_pcie_assert_reset(mtk_ctrl_info->port_base);
	mtk_phy_write_phase(target);
	mtk_pcie_relink(mtk_ctrl_info->port_base);
	pr_info("====Complete to configure best clock phase====\n");
}

static int mtk_get_value(char *str)
{
	int index, value = 0;
	char *format;

	if (str[0] == '0' && str[1] == 'x') {
		index = 2;
		format = "%x";
	} else {
		index = 0;
		format = "%d";
	}

	sscanf(&str[index], format, &value);
	return value;
}

static int mtk_get_info(int argc, char **argv)
{
	pr_info("Current port gpio_addr: 0x%x.\n", mtk_ctrl_info->gpio_addr);
	pr_info("Current port dir_offset 0x%x.\n", mtk_ctrl_info->gpio_dir_offset);
	pr_info("Current port dout_offset: 0x%x.\n", mtk_ctrl_info->gpio_dout_offset);
	pr_info("Current port din_offset: 0x%x.\n", mtk_ctrl_info->gpio_din_offset);
	pr_info("Current port pin_sda:%d.\n", mtk_ctrl_info->pin_sda);
	pr_info("Current port pin_scl:%d.\n", mtk_ctrl_info->pin_scl);
	pr_info("Current support max lane: %d.\n", mtk_ctrl_info->lane);
	pr_info("Current support max speed: GEN%d.\n", mtk_ctrl_info->speed);
	pr_info("Current target phase: 0x%x.\n", mtk_ctrl_info->phase);

	return 0;
}

static int mtk_board_init(int argc, char **argv)
{
	mtk_pcie_assert_reset(mtk_ctrl_info->port_base);
	mtk_phy_base_init(mtk_ctrl_info->lane);

	if (mtk_ctrl_info->phase == AUTO_SCAN)
		mtk_phase_scan();
	else
		mtk_phy_write_phase(mtk_ctrl_info->phase);

	mtk_pcie_relink(mtk_ctrl_info->port_base);

	return 0;
}

static int mtk_set_max_lane(int argc, char **argv)
{
	int lane = ONE;

	if (argc > 1) {
		if (!strcmp(argv[1], "1"))
			lane = ONE;
		else if (!strcmp(argv[1], "2"))
			lane = TWO;
		else if (!strcmp(argv[1], "4"))
			lane = FOUR;
	}

	mtk_ctrl_info->lane = lane;

	mtk_pcie_set_max_lane(mtk_ctrl_info->port_base, lane);

	mtk_pcie_assert_reset(mtk_ctrl_info->port_base);
	mtk_phy_switch_lane(lane);

	pr_info("Set support max lane is %d.\n", mtk_ctrl_info->lane);

	return 0;
}

static int mtk_set_max_speed(int argc, char **argv)
{
	int speed = GEN1;

	if (argc > 1) {
		if (!strcmp(argv[1], "5G"))
			speed = GEN2;
	}

	mtk_ctrl_info->speed = speed;

	mtk_pcie_set_max_speed(mtk_ctrl_info->port_base, speed);

	pr_info("Set support max speed is GEN%d.\n", mtk_ctrl_info->speed);

	return 0;
}

static int mtk_set_phase(int argc, char **argv)
{
	int phase;

	phase = 0xFF;

	if (argc > 1)
		phase = mtk_get_value(argv[1]);

	if (phase > 0x1f) {
		mtk_ctrl_info->phase = AUTO_SCAN;
		mtk_phase_scan();
	} else {
		mtk_ctrl_info->phase = phase;
		mtk_pcie_assert_reset(mtk_ctrl_info->port_base);
		mtk_phy_write_phase(phase);
		mtk_pcie_relink(mtk_ctrl_info->port_base);
	}

	pr_info("Set target phase is 0x%x.\n", mtk_ctrl_info->phase);

	return 0;
}

static int mtk_set_gpio_info(int argc, char **argv)
{
	if (argc > 1)
		mtk_ctrl_info->gpio_addr = mtk_get_value(argv[1]);
	else
		return 0;

	if (argc > 2)
		mtk_ctrl_info->gpio_dir_offset = mtk_get_value(argv[2]);

	if (argc > 3)
		mtk_ctrl_info->gpio_dout_offset = mtk_get_value(argv[3]);

	if (argc > 4)
		mtk_ctrl_info->gpio_din_offset = mtk_get_value(argv[4]);

	if (argc > 5)
		mtk_ctrl_info->pin_sda = mtk_get_value(argv[5]);

	if (argc > 6)
		mtk_ctrl_info->pin_scl = mtk_get_value(argv[6]);

	pr_info("Current GPIO info:\n");
	pr_info("gpio_addr is 0x%x.\n", mtk_ctrl_info->gpio_addr);
	pr_info("gpio_dir_offset is 0x%x.\n", mtk_ctrl_info->gpio_dir_offset);
	pr_info("gpio_dout_offset is 0x%x.\n", mtk_ctrl_info->gpio_dout_offset);
	pr_info("gpio_din_offset is 0x%x.\n", mtk_ctrl_info->gpio_din_offset);
	pr_info("pin_sda is %d.\n", mtk_ctrl_info->pin_sda);
	pr_info("pin_scl is %d.\n", mtk_ctrl_info->pin_scl);

	mtk_phy_exit(mtk_ctrl_info->full_name);

	mtk_pcie_assert_reset(mtk_ctrl_info->port_base);

	mtk_phy_init(mtk_ctrl_info->gpio_addr, mtk_ctrl_info->gpio_dir_offset,
		mtk_ctrl_info->gpio_dout_offset, mtk_ctrl_info->gpio_din_offset,
		mtk_ctrl_info->pin_sda, mtk_ctrl_info->pin_scl,
		mtk_ctrl_info->lane, mtk_ctrl_info->full_name);

	return 0;
}

struct cmd_tbl cmd_table[] = {
	{	"info",
		&mtk_get_info,
		"Get current port info."
	},
	{	"lane",
		&mtk_set_max_lane,
		"Set max lane support.[1/2/4]"
	},
	{	"speed",
		&mtk_set_max_speed,
		"Set max speed support.[2.5G/5G]"
	},
	{	"phase",
		&mtk_set_phase,
		"Set taget phase[<0x20] or auto scan best phase[0xff]"
	},
	{	"gpio",
		&mtk_set_gpio_info,
		"Set gpio info. [gpio_addr] [dir_offset] [dout_offset] [din_offset] [pin_sda] [pin_scl]"
	},
	{	"init",
		&mtk_board_init,
		"Complete re-init phy board."
	},
	{}
};

static int mtk_main_ctrl(char *buf)
{
	struct cmd_tbl *cmd_list = cmd_table;
	char *argv[10];
	int ret = 0;
	int argc = 0;

	do {
		argv[argc] = strsep(&buf, " ");
		pr_info("[%d] %s\r\n", argc, argv[argc]);
		argc++;
	} while (buf);

	while (cmd_list->name != NULL) {
		if (!strcmp(cmd_list->name, argv[0])) {
			if (cmd_list->cb_func != NULL) {
				ret = cmd_list->cb_func(argc, argv);
				return ret;
			}

			pr_err("Call back function not found.\n");
			break;
		}

		cmd_list++;
	}

	return -EINVAL;
}

static ssize_t cli_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	struct cmd_tbl *cmd = cmd_table;

	pr_info("Support commands:\n");

	while (cmd->name != NULL || cmd->help != NULL) {
		pr_info("%-30s %s\n", cmd->name, cmd->help);
		cmd++;
	}

	return 1;
}

static ssize_t cli_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t n)
{
	int i, ret = 0;
	char *ch;

	for (i = 0; i < 10; i++) {
		if ((mtk_ctrl[i]->full_name != NULL) && !strcmp(mtk_ctrl[i]->full_name, kobj->name)) {
			mtk_ctrl_info = mtk_ctrl[i];
			break;
		}
	}
	if (i == 10)
		return -EINVAL;

	mtk_phy_set_port(kobj->name);

	ch = kmalloc(n, GFP_KERNEL);
	memcpy(ch, buf, n);
	if (ch[n-1] == '\n')
		ch[n-1] = '\0';

	ret = mtk_main_ctrl(ch);
	if (ret < 0) {
		pr_info("phy cli fail\n");
		kfree(ch);
		return -EINVAL;
	}

	kfree(ch);
	pr_info("%s", buf);

	return n;
}

#define phy_attr(_name) \
	static struct kobj_attribute _name##_attr = {   \
		.attr	= {.name = __stringify(_name),  \
			   .mode = 0644,                \
		},                                      \
		.show	= _name##_show,                 \
		.store	= _name##_store,                \
	}
phy_attr(cli);

static struct attribute *main_func[] = {
	&cli_attr.attr,
	NULL,
};

static struct attribute_group main_kobj_group = {
	.attrs = main_func,
};

static int mtk_phy_parse(struct device *dev)
{
	struct device_node *node = dev->of_node;
	const __be32 *property;

	mtk_ctrl_info->full_name = node->full_name;

	property = of_get_property(node, "mediatek,i2c-gpio", NULL);
	if (!property) {
		pr_err("failed to get PHY board i2c gpio\n");
		return -EINVAL;
	}
	mtk_ctrl_info->gpio_addr = be32_to_cpup(property + 0);
	mtk_ctrl_info->gpio_dir_offset = be32_to_cpup(property + 1);
	mtk_ctrl_info->gpio_dout_offset = be32_to_cpup(property + 2);
	mtk_ctrl_info->gpio_din_offset = be32_to_cpup(property + 3);
	mtk_ctrl_info->pin_sda = be32_to_cpup(property + 4);
	mtk_ctrl_info->pin_scl = be32_to_cpup(property + 5);

	property = of_get_property(node, "mediatek,pcie-port", NULL);
	if (!property) {
		pr_err("failed to get PHY board PCIe port base\n");
		return -EINVAL;
	}
	mtk_ctrl_info->pcie_addr = be32_to_cpup(property + 0);
	mtk_ctrl_info->speed = be32_to_cpup(property + 1);
	mtk_ctrl_info->lane = be32_to_cpup(property + 2);
	mtk_ctrl_info->phase = be32_to_cpup(property + 3);

	mtk_ctrl_info->port_base = ioremap(mtk_ctrl_info->pcie_addr, 0x2000);
	if (!mtk_ctrl_info->port_base) {
		pr_err("mapping pcie mac addr failed\n");
		return -ENOMEM;
	}

	return 0;
}

static int mtk_top_init(struct device *dev)
{
	struct device_node *node = dev->of_node;
	const __be32 *property;
	void __iomem *vaddr;
	unsigned int addr, value, mask, tmp;
	int i, len;

	property = of_get_property(node, "mediatek,top-init", &len);
	if (!property) {
		pr_err("Can not get PHY board init setting\n");
		return -EINVAL;
	}

	for (i = 0; i < (len / (4 * 3)); i++) {
		addr = be32_to_cpup(property + i * 3 + 0);
		value = be32_to_cpup(property + i * 3 + 1);
		mask = be32_to_cpup(property + i * 3 + 2);
		vaddr = ioremap(addr, 0x4);
		tmp = readl_relaxed(vaddr);
		writel_relaxed(((tmp & ~mask) | (value & mask)), vaddr);
		iounmap(vaddr);
	}

	return 0;
}
static int mtk_main_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int i, ret = 0;

	mtk_ctrl_info = devm_kzalloc(dev, sizeof(*mtk_ctrl_info), GFP_KERNEL);
	for (i = 0; i < 10; i++) {
		if (mtk_ctrl[i] == NULL) {
			mtk_ctrl[i] = mtk_ctrl_info;
			break;
		}
	}
	if (i == 10)
		return -EINVAL;

	ret = mtk_phy_parse(dev);
	if (ret)
		return -EINVAL;

	mtk_top_init(dev);
	if (ret)
		return -EINVAL;

	mtk_pcie_init(mtk_ctrl_info->port_base, mtk_ctrl_info->speed,
				mtk_ctrl_info->lane);

	ret = mtk_phy_init(mtk_ctrl_info->gpio_addr,
				mtk_ctrl_info->gpio_dir_offset,
				mtk_ctrl_info->gpio_dout_offset,
				mtk_ctrl_info->gpio_din_offset,
				mtk_ctrl_info->pin_sda,
				mtk_ctrl_info->pin_scl,
				mtk_ctrl_info->lane,
				mtk_ctrl_info->full_name);

	if (ret)
		return ret;

	if (mtk_ctrl_info->phase == AUTO_SCAN)
		mtk_phase_scan();
	else
		mtk_phy_write_phase(mtk_ctrl_info->phase);


	/* sysfs support */
	mtk_ctrl_info->main_kobj = kobject_create_and_add(mtk_ctrl_info->full_name, NULL);
	if (!mtk_ctrl_info->main_kobj)
		return -ENOMEM;

	ret = sysfs_create_group(mtk_ctrl_info->main_kobj, &main_kobj_group);
	if (ret) {
		pr_info("Phy board sysfs creat fail\n");
		return ret;
	}

	return ret;
}

static int mtk_main_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int i;

	for (i = 0; i < 10; i++) {
		if ((mtk_ctrl[i]->full_name != NULL) && !strcmp(mtk_ctrl[i]->full_name, dev->of_node->full_name)) {
			mtk_ctrl_info = mtk_ctrl[i];
			mtk_ctrl[i] = NULL;
			break;
		}
	}
	if (i == 10)
		return -EINVAL;

	iounmap(mtk_ctrl_info->port_base);
	mtk_ctrl_info->port_base = NULL;
	mtk_phy_exit(mtk_ctrl_info->full_name);

	/* remove sysfs */
	sysfs_remove_group(mtk_ctrl_info->main_kobj, &main_kobj_group);
	kobject_del(mtk_ctrl_info->main_kobj);

	return 0;
}

static const struct of_device_id mtk_pcie_of_match[] = {
	{ .compatible = "mediatek,fpga-pcie-phy" },
	{},
};

static struct platform_driver mtk_pcie_driver = {
	.probe = mtk_main_probe,
	.remove = mtk_main_remove,
	.driver = {
		.name = "mtk-fpga-pcie-phy",
		.of_match_table = mtk_pcie_of_match,
	},
};

module_platform_driver(mtk_pcie_driver);
MODULE_LICENSE("GPL v2");
