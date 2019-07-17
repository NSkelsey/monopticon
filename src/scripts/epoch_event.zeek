# This program generates router identification events on a schedule

#@load policy/protocols/conn/mac-logging
#@load policy/protocols/conn/vlan-logging
@load policy/misc/stats.bro

export {
  type L2Summary: record {
    # maps enum: L3_IPV4, L3_IPV6, L3_ARP, L3_UNKOWN to num_pkts
    ipv4: count;
    ipv6: count;
    arp: count;
    unknown: count;
  };

  type DeviceComm: record {
    mac_src: string;

    # key mac_dst
    tx_summary: table[string] of L2Summary;
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
  };

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
    #enter_routers: set[Router];

    l2_dev_comm: table[string] of DeviceComm;
    #moment: time;

    #exit_routers: set[string];
    #exit_devices: set[string];
  };

  # mac_src -> L2Dev
  global L2DeviceTable: table[string] of L2Device;
  # mac_src -> Router
  global RouterTable: table[string] of Router;

  #global tick_resolution = 250usec;
  global tick_resolution = 15msec;

  # Internal state refreshed every Epoch:
  # mac_src key
  global epoch_new_l2devices: set[string];

  # mac_src key
  global epoch_l2_dev_comm: table[string] of DeviceComm;

  redef Stats::report_interval = 250msec;
}



event epoch_fire(m: EpochStep) {}

event epoch_step() {

  local msg: EpochStep;

  msg$enter_l2devices = epoch_new_l2devices;

  msg$l2_dev_comm = epoch_l2_dev_comm;

  #print "===============";
  #print fmt("new devs: %s, num comms: %s", |msg$enter_l2devices|, |msg$l2_dev_comm|);

  # TODO(mem mgmt) delete everything iteratively from sets
  epoch_new_l2devices = set();
  epoch_l2_dev_comm = table();

  event epoch_fire(msg);

  schedule tick_resolution { epoch_step() };
}


function create_L2Device(mac_src: string): L2Device
{
  local dev: L2Device;
  dev$mac = mac_src;
  dev$emitted_ipv4 = set();
  dev$emitted_ipv6 = set();
  dev$arp_table = table();
  add epoch_new_l2devices[mac_src];
  L2DeviceTable[mac_src] = dev;
  return dev;
}

function create_DeviceComm(mac_src: string): DeviceComm
{
  local comm: DeviceComm;
  comm$mac_src = mac_src;
  comm$tx_summary = table();
  epoch_l2_dev_comm[mac_src] = comm;
  return comm;
}

function create_L2Summary(): L2Summary
{
  local summary: L2Summary;
  summary$ipv4=0;
  summary$ipv6=0;
  summary$arp=0;
  summary$unknown=0;

  return summary;
}

function update_comm_table(comm: DeviceComm, p: raw_pkt_hdr) {
  local summary: L2Summary;
  if (p$l2$dst !in comm$tx_summary) {
    summary = create_L2Summary();
  } else {
    summary = comm$tx_summary[p$l2$dst];
  }
  comm$tx_summary[p$l2$dst] = summary;

  switch p$l2$proto
  {
    case L3_ARP:
      summary$arp += 1; break;
    case L3_IPV4:
      summary$ipv4 += 1; break;
    case L3_IPV6:
      summary$ipv6 += 1; break;
    case L3_UNKNOWN:
      summary$unknown += 1; break;
  }
}

event raw_packet(p: raw_pkt_hdr)
{
  if (p?$l2 && p$l2?$src) {
    local mac_src = p$l2$src;
    local mac_dst = p$l2$dst;

    local dev: L2Device;
    # TODO(sec/scalability) add mac spoofing protection here
    if (mac_src !in L2DeviceTable) {
      dev = create_L2Device(mac_src);
    } else {
      dev = L2DeviceTable[mac_src];
    }

    # TODO(idem) prevent mac_dst spray
    if (mac_dst !in L2DeviceTable) {
      create_L2Device(mac_dst);
    }

    local comm: DeviceComm;
    if (mac_src !in epoch_l2_dev_comm) {
        comm = create_DeviceComm(mac_src);
    } else {
        comm = epoch_l2_dev_comm[mac_src];
    }
    update_comm_table(comm, p);

    if (p?$ip && p$ip$src !in dev$emitted_ipv4) {
        add dev$emitted_ipv4[p$ip$src];
    }

    if (p?$ip6 && p$ip6$src !in dev$emitted_ipv6) {
        add dev$emitted_ipv6[p$ip6$src];
    }
  } else {
    # TODO log non captured pkts
    # wierd +=1;
    ;
  }
}

event zeek_init()
{

  print "Trying to add peer";
  Broker::peer("127.0.0.1", 9999/tcp, 0sec);
  print "Starting epoch_steps";
}

event Broker::peer_added(endpoint: Broker::EndpointInfo, msg: string)
{
    print "peer added", endpoint;
    Broker::auto_publish("monopt/l2", epoch_fire);
    Broker::auto_publish("monopt/stats", Stats::log_stats);
    #last_tick = network_time();
    schedule tick_resolution { epoch_step() };
}
