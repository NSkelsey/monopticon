# This program analyzes a pcap to hueristically determine vlan tags
# Usage: zeek -r <pcap_filen> ./identify_routers.zeek

@load policy/protocols/conn/known-hosts
@load policy/protocols/conn/mac-logging
@load policy/protocols/conn/vlan-logging

module Routers;

export {
    global local_nets = {
       192.168.0.0/16,
       127.0.0.0/8,
       172.16.0.0/12,
       10.0.0.0/8,
       100.64.0.0/10
    };

    global mac_src_ip_emitted: table[string] of set[addr];
    global mac_src_vlan_emitted: table[string] of set[count];

    global vlan_ip_emitted: table[count] of set[addr];
    global vlan_ip_strange: table[count] of set[addr];

    global mac_src_routing_table: table[string] of table[string] of addr;

    global find_link_local: function(p: bool): count;
    global build_vlans: function(vlan_ip_tbl_set: table[count] of set[addr], p: bool) : table[count] of subnet;
    global find_routers: function(mac_src_ip: table[string] of set[addr], threshold: count, p: bool): table[subnet] of string;

}

event raw_packet(p: raw_pkt_hdr) {
    # Check if the packet is a vlan tagged ipv4 packet inside of an ethernet frame
    # If so add it to the data structures defined above
    if (p?$ip && p$l2?$src && p$l2?$vlan) {
        local mac_src= p$l2$src;
        local ip_src = p$ip$src;
        local vlan = p$l2$vlan;

        local s: set[addr];
        local g: set[count];
        if (mac_src !in mac_src_ip_emitted) {
            mac_src_ip_emitted[mac_src] = s;
            mac_src_vlan_emitted[mac_src] = g;
        } else {
            s = mac_src_ip_emitted[mac_src];
            g = mac_src_vlan_emitted[mac_src];
        }
        add s[ip_src];
        add g[vlan];

        local h: set[addr];
        if (vlan !in vlan_ip_emitted) {
            vlan_ip_emitted[vlan] = h;
            local t: set[addr];
            vlan_ip_strange[vlan] = t;
        } else {
            h = vlan_ip_emitted[vlan];
        }
        add h[ip_src];
    }
}

function infer_subnet(ip_set: set[addr]): subnet {
    # Create an ip mask using a bitwise 'and' across all ips in the passed set
    local iv: index_vec = [4294967295];
    for (_ip in ip_set) {
        local c: index_vec = addr_to_counts(_ip);
        iv[0] = iv[0] & c[0];
    }

    local b = floor(log10(|ip_set|)/log10(2))+6;
    local snet_mask = double_to_count(b);
    local pos_snet = counts_to_addr(iv);

    # Try to infer an appropriate subnet mask based on the actual number
    # of hosts seen inside of the network. This is a hueristic.
    local snet = mask_addr(pos_snet, 32-snet_mask);

    return snet;
}

function build_vlans(vlan_ip_tbl_set: table[count] of set[addr], p: bool) : table[count] of subnet {
    # This function constructs possible vlans based on the src ip addresses
    # and their corresponding vlan tag. It will produce results only as good as
    # the input.
    local vlan_subnets: table[count] of subnet;

    for (_vlan in vlan_ip_tbl_set) {
        local set_ip = vlan_ip_tbl_set[_vlan];
        local strange = vlan_ip_strange[_vlan];

        # filter and track all observed IPs that are not part of local_networks
        for (_ip in set_ip) {
            if (addr_to_subnet(_ip) !in local_nets) {
                add strange[_ip];
            }
        }

        set_ip = set_ip - strange;

        local snet = infer_subnet(set_ip);

        vlan_subnets[_vlan] = snet;

        if (p) {
            print "vlan", _vlan, snet, |set_ip|;
            if (|strange| > 0) {
                print "non local to vlan", _vlan, strange;
            }
        }
    };



    return vlan_subnets;
}

function find_routers(mac_src_ip: table[string] of set[addr], threshold: count, p: bool): table[subnet] of string {
    # List all mac addrs with more than threshold ip
    local r_t: table[subnet] of string;

    for (mac_src in mac_src_ip) {
        local ip_set = mac_src_ip[mac_src];

        for (_ip in ip_set) {
            if (_ip !in local_nets) {
                delete ip_set[_ip];
            }
        }
        if (|ip_set| > threshold) {
            local sn = infer_subnet(ip_set);
            if (sn in r_t && p) {
                print "subnet", sn, "not unique!";
            }
            r_t[sn] = mac_src;
        }
    }

    return r_t;
}

function find_link_local(p: bool): count {
    # List all mac addrs with just one src ip

    local cnt = 0;
    for (mac_src in mac_src_ip_emitted) {
        local ip_set = mac_src_ip_emitted[mac_src];
        if (|ip_set| == 1) {
            cnt += 1;
        }
    }
    return cnt;
}

function output_summary() {
    print "Observed subnets:";
    local vlan_subnets = build_vlans(vlan_ip_emitted, F);
    print vlan_subnets;
    print "";
    print "Routers with subnets behind:";
    find_routers(T);
    local cnt = find_link_local(T);
    print "";
    print fmt("Seen: %d devices that could be link local", cnt);
}
