# This program generates router identification events on a schedule

@load policy/protocols/conn/known-hosts
@load policy/protocols/conn/mac-logging
@load policy/protocols/conn/vlan-logging

export {
    type L2Conn: record {
        mac_dst: string;

        # table enum: L3_IPV4, L3_IPV6, L3_ARP, L3_UNKOWN to num_pkts
        l3_counts: table[layer3_proto] of count;
    };

    type DeviceCom: record {
        mac_src: string;

        tx_conn_streams: set[L2Conn];
    };

    # Internal
    type L2Device: record {
        mac: string;

        # All seen IPs used from the dev
        emitted_ipv4: set[addr];

        # idem
        emitted_ipv6: set[addr];

        # Table that maps inferred routes
        # mac_dst -> subnet
        arp_table: table[string] of subnet;
    }

    # Internal
    type Router: record {
        mac: string;
        snet: subnet;
        unique_hosts: count;
        tot_rx_pkts: count;
        tot_tx_pkts: count;
    };

    type EpochStep: record {
        enter_l2devices: set[string];
        enter_routers: set[Router];

        communications: set[DeviceCom];
        moment: time;

        exit_routers: set[string];
        exit_devices: set[string];
    };

    # mac_src -> L2Dev
    global L2DeviceTable: table[string] of L2Device;
    # mac_src -> Router
    global RouterTable: table[string] of Router;

    global events_per_second: count 240;
}

function aggregate_l2_src(ts: time, key: SumStats::Key, result: SumStats::Result)
{
    # TODO local all_devices: set[string];
    # filter from windowed device tracking
        # use 1 minute to expire devices
    local half_conn: L2Conn;
    half_conn$enter = network_time();


    step$enter_devices = all_devices - result['l2_conn.src'];

    # Transform observed edges with unique src ips
    local result['l2_conn.src.ip'];
    local possible_routers = find_routers[, 4, F];
    step$enter_routers = all_routers - possible_routers;

}

event zeek_init()
{
    local r1: SumStats::Reducer = [$stream="l2_conn",
                                   $apply=set(SumStats::UNIQUE)];
    local r2: SumStats::Reducer = [$stream="l2_conn.src",
                                   $apply=set(SumStats::UNIQUE)];
    local r3: SumStats::Reducer = [$stream="l2_conn.src.ip",
                                   $apply=set(SumStats::UNIQUE)];
    local r4: SumStats::Reducer = [$stream="l2_conn.pkts",
                                   $apply=set(SumStats::SUM)];
    local r5: SumStats::Reducer = [$stream="l2_conn.types",
                                   $apply=set(SumStats::UNIQUE)];
    local r6: SumStats::Reducer = [$stream="l2_conn.dup_socket",
                                   $apply=set(SumStats::UNIQUE)];
    SumStats::create([$name="epoch_step",
                      $epoch=break_interval,
                      $reducers=set(r1, r2, r3, r4, r4, r5, r6),
                      $epoch_result=clock_tick]);
}

event raw_packet(p: raw_packet_hdr)
{
    if ((p?$l2) && (p?$l2?$src)) {
        local mac = p$l2$src;
        local mac_dst = p$l2$dst;
        local l2_conn = mac_src + '|' + mac_dst;
        SumStats::observe("l2_conn", [$str=l2_conn, []);
        SumStats::observe("l2_conn.pkts", [$str=l2_conn, [$str=]];
        SumStats::observe("l2_conn.types", [$str=l2_conn, [$str=]];

        if (p?$ip) {
            SumStats::observe("l2_conn.src.ip", [$str=l2_conn, [$addr=p$ip$src]);
        }
    }
}
