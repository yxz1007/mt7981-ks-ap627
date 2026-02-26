#!/bin/bash
cd ~/mt798x/openwrt

echo "修复缺失依赖..."

# 1. 更新所有 feeds
./scripts/feeds update -a

# 2. 搜索可用的替代包
echo "搜索 kmod-nf-flow-netlink 的替代包..."
./scripts/feeds search kmod-nf-flow

echo "搜索 kmod-crypto-chacha20poly1305 的替代包..."
./scripts/feeds search chacha20

echo "搜索 libwpactrl 的替代包..."
./scripts/feeds search wpa

# 3. 修改有问题的 Makefile
echo "修改有问题的 Makefile..."

# 修改 flowtable Makefile
if [ -f "package/feeds/mtk_openwrt_feed/flowtable/Makefile" ]; then
    echo "修改 flowtable Makefile..."
    sed -i 's/kmod-nf-flow-netlink/kmod-nf-flow/g' package/feeds/mtk_openwrt_feed/flowtable/Makefile
fi

# 修改 wappd Makefile
if [ -f "package/mtk/applications/wappd/Makefile" ]; then
    echo "修改 wappd Makefile..."
    sed -i 's/libwpactrl/wpa-cli/g' package/mtk/applications/wappd/Makefile
fi

# 4. 安装可能需要的包
echo "安装可能需要的包..."
./scripts/feeds install kmod-nf-flow
./scripts/feeds install wpa-cli
./scripts/feeds install strongswan

# 5. 在 menuconfig 中启用必要的包
echo "请运行 make menuconfig 手动配置以下选项："
echo "1. Network -> Firewall -> 选择 kmod-nf-flow"
echo "2. Kernel modules -> Cryptographic API modules -> 选择 kmod-crypto-chacha20poly1305"
echo "3. Utilities -> 选择 wpa-cli"
echo "4. 取消选择可能导致循环依赖的包"

echo "依赖修复完成"
