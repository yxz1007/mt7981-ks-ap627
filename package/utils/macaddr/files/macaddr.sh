#!/bin/sh
FACTORY="/dev/mtd3"
OFF_RA0=4
OFF_ETH1=36
OFF_ETH0=42

read_mac() {
    dd if="$FACTORY" bs=1 skip="$1" count=6 2>/dev/null | \
    hexdump -e '6/1 "%02x:"' | sed 's/:$//' | tr 'a-f' 'A-F'
}

write_byte_to_file() {
    local file="$1"
    local off="$2"
    local hex="$3"
    local dec=$((0x$hex))
    local oct=$(printf '%03o' $dec)
    printf "\\$oct" | dd of="$file" bs=1 seek=$off conv=notrunc 2>/dev/null
}

write_all_macs() {
    local base="$1"
    local eth0="$2"
    local eth1="$3"
    
    cp "$FACTORY" /tmp/factory_new.bin 2>/dev/null
    
    local hex_base=$(echo "$base" | tr -d ':')
    local hex_eth0=$(echo "$eth0" | tr -d ':')
    local hex_eth1=$(echo "$eth1" | tr -d ':')
    
    local i=0
    while [ $i -lt 12 ]; do
        write_byte_to_file /tmp/factory_new.bin $((OFF_RA0+i/2)) "${hex_base:$i:2}"
        write_byte_to_file /tmp/factory_new.bin $((OFF_ETH0+i/2)) "${hex_eth0:$i:2}"
        write_byte_to_file /tmp/factory_new.bin $((OFF_ETH1+i/2)) "${hex_eth1:$i:2}"
        i=$((i+2))
    done
    
    echo "Erasing Factory..."
    mtd erase Factory 2>/dev/null
    
    echo "Writing Factory..."
    mtd write /tmp/factory_new.bin Factory 2>/dev/null
    
    rm -f /tmp/factory_new.bin
    rm -f /lib/firmware/e2p
}

validate_mac() {
    local mac="$1"
    echo "$mac" | grep -qE '^([0-9A-Fa-f]{2}[:-]){5}[0-9A-Fa-f]{2}$' || \
    echo "$mac" | grep -qE '^[0-9A-Fa-f]{12}$' || {
        echo "Error: Invalid MAC format. Use XX:XX:XX:XX:XX:XX"
        exit 1
    }
    echo "$mac" | tr 'a-f' 'A-F' | tr -d ':-' | sed 's/../&:/g;s/:$//'
}

mac_inc() {
    local num=$(echo "$1" | tr -d ':')
    printf '%012X' $((0x$num + $2)) | sed 's/../&:/g;s/:$//'
}

apply_uci() {
    # 修正 UCI 里硬编码的旧 MAC
    local base="$1"
    local eth0="$2"
    local eth1="$3"
    
    # lan1-4 用 base MAC（和 ra0 相同，br-lan 继承）
    for idx in 1 2 3 4; do
        uci set network.@device[$idx].macaddr="$base" 2>/dev/null
    done
    
    # eth1 = WAN
    uci set network.@device[5].macaddr="$eth1" 2>/dev/null
    
    # eth0 新增 device 节点（原来没有）
    # 检查是否已有 eth0 的 device 节点
    local eth0_idx=$(uci show network | grep -n "name='eth0'" | head -1 | cut -d: -f1)
    if [ -z "$eth0_idx" ]; then
        # 没有就新增
        uci add network device
        uci set network.@device[-1].name='eth0'
        uci set network.@device[-1].macaddr="$eth0"
    else
        # 有就改
        # 解析出 device[N] 的索引
        local real_idx=$(uci show network | grep "name='eth0'" | head -1 | sed "s/.*@device\[\([0-9]*\)\].*/\1/")
        uci set network.@device[$real_idx].macaddr="$eth0"
    fi
    
    uci commit network
}

case "$1" in
    set)
        [ -z "$2" ] && { echo "Usage: macaddr set XX:XX:XX:XX:XX:XX"; exit 1; }
        BASE=$(validate_mac "$2")
        ETH0=$(mac_inc "$BASE" 1)
        ETH1=$(mac_inc "$BASE" 2)
        
        echo "Setting:"
        echo "  ra0:  $BASE"
        echo "  eth0: $ETH0"
        echo "  eth1: $ETH1"
        
        write_all_macs "$BASE" "$ETH0" "$ETH1"
        
        echo "Verify:"
        echo "  ra0:  $(read_mac $OFF_RA0)"
        echo "  eth0: $(read_mac $OFF_ETH0)"
        echo "  eth1: $(read_mac $OFF_ETH1)"
        
        fw_setenv ethaddr "$ETH0"
        fw_setenv eth1addr "$ETH1"
        
        apply_uci "$BASE" "$ETH0" "$ETH1"
        
        echo "Done. Reboot."
        ;;
    get)
        echo "Factory:"
        echo "  ra0:  $(read_mac $OFF_RA0)"
        echo "  eth0: $(read_mac $OFF_ETH0)"
        echo "  eth1: $(read_mac $OFF_ETH1)"
        echo "System:"
        for i in eth0 eth1 br-lan lan1 lan2 lan3 lan4 ra0 rax0; do
            [ -e /sys/class/net/$i ] && printf "  %-6s: %s\n" "$i" "$(cat /sys/class/net/$i/address)"
        done
        ;;
    restore)
        BASE=$(read_mac $OFF_RA0)
        ETH0=$(read_mac $OFF_ETH0)
        ETH1=$(read_mac $OFF_ETH1)
        
        # 设置运行中接口的 MAC
        ip link set dev eth0 address "$ETH0" 2>/dev/null
        ip link set dev eth1 address "$ETH1" 2>/dev/null
        
        apply_uci "$BASE" "$ETH0" "$ETH1"
        
        # 重启网络生效
        /etc/init.d/network restart 2>/dev/null
        
        echo "BASE=$BASE"
        echo "ETH0=$ETH0"
        echo "ETH1=$ETH1"
        ;;
    *)
        echo "Usage: macaddr set|get|restore"
        ;;
esac