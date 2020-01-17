#!/bin/bash
# This script allows the customization of the underlying system interactions
# that monopticon uses to connect with zeek.

############# Modifiable Functions #############

list_ifaces() {
    EXCLUDED_INTERFACES=("lo", "enp0s31f6")

    # For each interface active
    for interface in `ip addr | egrep "UP|UNKNOWN" | grep -oP "([a-zA-Z][a-zA-Z0-9]+\d+)(?=:.+)"`; do
        if `contains  $interface "${EXCLUDED_INTERFACES[@]}"`; then
            continue
        fi
        interface_ip=`ip addr show $interface | grep -oP "(\d+\.\d+.\d+.\d+)(?=/\d+.+)"`
        echo -e "$interface"
    done
}

mac_addr() {
    if [ -z "$1" ]
    then
        echo "Must specify interface"
        return 0
    fi
    iface=$1

    mac=`ip addr show $iface | grep -oP "([a-f0-9]{2}:){5}([a-f0-9]{2})" | grep -Pv "ff:ff:ff:ff:ff:ff" | head -n1`
    echo -en "$mac"
}


ipv4_addr() {
    if [ -z "$1" ]
    then
        echo "Must specify interface"
        return 0
    fi
    iface=$1
    interface_ip=`ip addr show $iface | grep -oP '(\d+\.\d+.\d+.\d+)(?=/\d+.+)' | grep -Pv '127.0.0.1' | head -n1`
    echo -en "$interface_ip"
}

gateway_ipv4_addr() {
    if [ -z "$1" ]
    then
        echo "Must specify interface"
        return 0
    fi
    iface=$1
    gateway_ip=`ip route show default dev $iface | grep -oP "(\d+\.\d+.\d+.\d+)"`

    echo -en "$gateway_ip"
}

gateway_mac_addr() {
    if [ -z "$1" ]
    then
        echo "Must specify interface"
        return 0
    fi
    iface=$1
    gateway_ip=$2

    gateway_mac=`ip neighbour show dev $iface | grep $gateway_ip | grep REACHABLE | grep -oP "([a-f0-9]{2}:){5}([a-f0-9]{2})"`

    echo -en "$gateway_mac"
}

launch() {
    if [ -z "$1" ]
    then
        echo "Must specify interface"
        return 0
    fi
    iface=$1
    zeek_path="/opt/zeek/bin/zeek"
    zeek_lib_path="/opt/zeek/lib"
    script_path="/usr/local/share/monopticon/scripts/epoch_event.zeek $zeek_lib_path/policy/misc/stats.zeek"
    $zeek_path -i $iface -b $script_path >/dev/null &
    echo $!
}

sstop() {
    if [ -z "$1" ]
    then
        echo "Must specify PID of zeek listener"
        return 0
    fi
    pid=$1
    kill $pid
}

############# Utility Functions #############

function contains() {
    local e match="$1"
    shift
    for e; do
      [[ "$e" == "$match" ]] && return 0;
    done
    return 1
}

function usage() {
    echo "USAGE: monopt_iface_proto FUNC"
    echo "FUNC:="
    echo "     {launch <iface>}"
    echo "     {sstop <pid>}"
    echo "     {list_ifaces}"
    echo "     {mac_addr <iface>}"
    echo "     {ipv4_addr <iface>}"
    echo "     {gateway_ipv4_addr <iface>}"
    echo "     {gateway_mac_addr <iface>}"
}


# Check if the function exists (bash specific)
if declare -f "$1" > /dev/null
then
  # call arguments verbatim
  "$@"
else
  if [ -z "$1" ]
  then
    echo "No arguments supplied"
  else
  # Show a helpful error
  echo "'$1' is not a known function name" >&2
  fi
  usage
  exit 1
fi
