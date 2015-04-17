#/bin/bash 
# Get all interfaces presented
for i in `grep -woE "NAME=\".*\"" /etc/udev/rules.d/70-persistent-net.rules | grep -oE "[a-z0-9].*[^\"]"`;
do
    interfaces="$interfaces $i"
done

# Print interfaces
for i in $interfaces; do
    echo $i
done

function save_routes() {
    `ip route > monitor.route`
}

function compare_routes() {
    tmp=`cat monitor.route`
    new_routes=`ip route`
    if [ "$tmp" == "$new_routes" ]; then
        echo "routes not changed"
    else
        echo "route has been changed"
    fi
}

# Save IP per interface into IP_DUMP_FILE 
IP_DUMP_FILE="ip.dump"
function save_IP_addr() {
    echo "" > $IP_DUMP_FILE
    for interface in $interfaces; do
        tmp=`ip -o -4 addr list $interface | awk '{print \$4}' | cut -d\/ -f1`
        echo $interface $tmp >> $IP_DUMP_FILE
    done 
}

function compare_IP_addr() {
   for interface in $interfaces; do
       new_ip_addr=`ip -o -4 addr list $interface | awk '{print \$4}' | cut -d\/ -f1`
       old_ip_addr=`cat monitor.dump | grep $interface | grep -oE [0-9]{2}.+`
       #echo "NEW: \"$new_ip_addr\" OLD: \"$old_ip_addr\""
       if [ $new_ip_addr == $old_ip_addr ]; then
           echo "[$interface] IP not changed"
       else
           echo "[$interface] IP changed"
       fi

   done 
}

# unused
function start_IP_monitor() {
    ip monitor >> monitor.dump &
    monitor_pid=$!
    echo "Monitor PID = $monitor_pid"
    echo $monitor_pid > monitor.pid
    echo "Monitor with PID = $monitor_pid started"
}

# unused
function stop_IP_monitor() {
    monitor_pid=`cat monitor.pid`
    kill $monitor_pid
    echo "Monitor with PID = $monitor_pid killed"
    rm monitor.pid
}

function test_MAC_address_change() {
    info_dir="/sys/class/net"
    for interface in $interfaces; do
        mac=`cat ${info_dir}"/${interface}/address"`
        #echo "MAC address:$mac "
        if echo "$mac" | grep -q "$rules"; then
            echo "[$interface] MAC address not changed"
        else
            echo "[$interface] MAC address changed"
        fi
    done
}

function test_PCI_address_change() {
    info_dir="/sys/class/net"
    for interface in $interfaces; do 
        vendor=`cat ${info_dir}"/${interface}/device/vendor"`
        dev=`cat ${info_dir}"/${interface}/device/device"`
        pci_id=$vendor:$dev
        #echo "PCI address: $pci_id"
        if echo "$pci_id" | grep -q "$rules"; then
            echo "[$interface] PCI address not changed"
        else
            echo "[$interface] PCI address changed"
        fi
    done
}

echo "Test in on air"
test_PCI_address_change
test_MAC_address_change
save_IP_addr
save_routes
read
compare_IP_addr
compare_routes
