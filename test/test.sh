#/bin/bash 
# Get all interfaces presented
for i in `grep -woE "NAME=\".*\"" /etc/udev/rules.d/70-persistent-net.rules | grep -oE "[a-z0-9].*[^\"]"`;
do
    interfaces="$interfaces $i"
done

for i in $interfaces; do
    echo $i
done

function test_IP_not_changed() {
}

function test_MAC_address_change() {
    info_dir="/sys/class/net"
    for interface in $interfaces; do
        echo "[$interface]"
        mac=`cat ${info_dir}"/${interface}/address"`
        echo "MAC address:$mac "
        if echo "$mac" | grep -q "$rules"; then
            echo "MAC address not changed"
        else
            echo "MAC address changed"
        fi
    done
}

function test_PCI_address_change() {
    info_dir="/sys/class/net"
    for interface in $interfaces; do
        echo "[$interface]"
        vendor=`cat ${info_dir}"/${interface}/device/vendor"`
        dev=`cat ${info_dir}"/${interface}/device/device"`
        pci_id=$vendor:$dev
        echo "PCI address: $pci_id"
        if echo "$pci_id" | grep -q "$rules"; then
            echo "PCI address not changed"
        else
            echo "PCI address changed"
        fi
    done
}

echo "Test in on air"
test_PCI_address_change
test_MAC_address_change
