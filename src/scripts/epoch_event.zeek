# This program generates router identification events on a schedule

#@load policy/protocols/conn/mac-logging
#@load policy/protocols/conn/vlan-logging
@load policy/misc/stats.bro

export {
  global local_nets = {
    192.168.0.0/16,
    127.0.0.0/8,
    172.16.0.0/12,
    10.0.0.0/8,
    100.64.0.0/10
  };

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

    bcast_33: L2Summary &optional;
    bcast_ff: L2Summary &optional;
    bcast_01: L2Summary &optional;
    bcast_XX: L2Summary &optional;
  };

  # Internal
  type L2Device: record {
    mac: string;

    # All src IPs used from the dev
    src_emitted_ipv4: set[addr];

    # All dst IPs sent from the dev
    dst_emitted_ipv4: table[string] of set[addr];

    # idem
    emitted_ipv6: set[addr];

    # Table that maps inferred routes
    # mac_dst -> subnet
    arp_table: table[string] of subnet;

    group: bool;
    label: string;
    new_src_ip_ctr: count;
    new_dst_ip_ctr: count;
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
    # enter_routers: set[Router];
    # TODO enter_prefix_group: set[string];

    l2_dev_comm: table[string] of DeviceComm;
    # moment: time;

    enter_l2_ipv4_addr_src: table[string] of addr;
    enter_l2_ipv4_addr_dst: table[string] of addr;
    # exit_routers: set[string];
    # exit_devices: set[string];
  };

  # mac_src -> L2Dev
  global L2DeviceTable: table[string] of L2Device;
  # mac_src -> Router
  global RouterTable: table[string] of Router;

  # Note goal of this is to produce single dynamic table for controlling labelling and polymorphic objs under
  # vector of [{}, {'ff': L2BcastPool, '33' : L2Dev}, {}, {'00:04': L2PrefixGroup}]
  global PrefixVec: vector of table[string] of L2Device;

  #global tick_resolution = 250usec;
  global tick_resolution = 15msec;

  # Internal state refreshed every Epoch:
  # mac_src key
  global epoch_new_l2devices: set[string];

  # mac_src key
  global epoch_l2_dev_comm: table[string] of DeviceComm;

  type Result: record {
    ok: bool;
    v: any &optional;
  };

  redef Stats::report_interval = 250msec;
}


event epoch_fire(m: EpochStep) {
}

event epoch_step()
{

  local msg: EpochStep;

  msg$enter_l2devices = epoch_new_l2devices;

  msg$l2_dev_comm = epoch_l2_dev_comm;

  # TODO(mem mgmt) delete everything iteratively from sets
  epoch_new_l2devices = set();
  epoch_l2_dev_comm = table();

  for (mac_src in L2DeviceTable) {
    local dev = L2DeviceTable[mac_src];

    if (dev$new_src_ip_ctr > 0) {
      dev$new_src_ip_ctr = 0;
      for (ip_src_addr in dev$src_emitted_ipv4) {
        msg$enter_l2_ipv4_addr_src[mac_src] = ip_src_addr;
      }
    }

    if (dev $new_dst_ip_ctr > 0) {
      dev$new_dst_ip_ctr = 0;
    }

  }

  event epoch_fire(msg);

  schedule tick_resolution { epoch_step() };
}


function create_L2Device(mac_src: string, is_group: bool): L2Device
{
  local dev: L2Device;
  dev$group = is_group;
  dev$mac = mac_src;
  dev$src_emitted_ipv4 = set();
  dev$emitted_ipv6 = set();
  dev$arp_table = table();

  dev$new_src_ip_ctr = 0;
  dev$new_dst_ip_ctr = 0;

  add epoch_new_l2devices[mac_src];
  if (!is_group) {
    L2DeviceTable[mac_src] = dev;
  }
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

function is_broadcast(significant_bytes: string): bool
{
  return significant_bytes[1] == /1|3|5|7|9|b|d|f/;
}

function try_to_match_prefix(mac_src: string): Result
{
  for (i in PrefixVec) {
    local prefix_table = PrefixVec[i];
    local sig_bytes = mac_src[0:i+1];
    if (|prefix_table| == 0) {
      next;
    }
    for (prefix in prefix_table) {
      local l2dev = prefix_table[prefix];
      if (prefix == sig_bytes) {
        return [$ok=T, $v=l2dev];
      }
    }
  }
  return [$ok=F];
}

function get_bcast_summary(sig_bytes: string, comm: DeviceComm): L2Summary
{
  if (sig_bytes == /ff/) {
    if (!comm?$bcast_ff) {
      comm$bcast_ff = create_L2Summary();
    }
    return comm$bcast_ff;
  }
  if (sig_bytes == /33/) {
    if (!comm?$bcast_33) {
       comm$bcast_33 = create_L2Summary();
    }
    return comm$bcast_33;
  }
  if (sig_bytes == /01/) {
    if (!comm?$bcast_01) {
       comm$bcast_01 = create_L2Summary();
    }
    return comm$bcast_01;
  } else {
    if (!comm?$bcast_XX) {
      comm$bcast_XX = create_L2Summary();
    }
    return comm$bcast_XX;
  }
}

function update_comm_table(comm: DeviceComm, p: raw_pkt_hdr): bool
{
  local summary: L2Summary;
  local use_l2_dev_table = T;
  local mac_dst = p$l2$dst;
  local sig_bytes = mac_dst[0:2];

  if (is_broadcast(sig_bytes)) {
    use_l2_dev_table = F;
    summary = get_bcast_summary(sig_bytes, comm);
  } else {
    local res = try_to_match_prefix(mac_dst);
    if (res$ok) {
        local dev: L2Device = res$v;
        mac_dst = dev$mac;
        use_l2_dev_table = F;
    }

    if (mac_dst !in comm$tx_summary) {
      summary = create_L2Summary();
    } else {
      summary = comm$tx_summary[mac_dst];
    }
    comm$tx_summary[mac_dst] = summary;
  }

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

  return use_l2_dev_table;
}

event raw_packet(p: raw_pkt_hdr)
{
  if (p?$l2 && p$l2?$src) {
    local mac_src = p$l2$src;
    local mac_dst = p$l2$dst;

    local dev: L2Device;

    # TODO Match the prefix of the mac against existing groups
    local res = try_to_match_prefix(mac_src);
    if (res$ok) {
      dev = res$v;
      mac_src = dev$mac;
    } else {
      # TODO(sec/scalability) add mac spoofing protection here
      if (mac_src !in L2DeviceTable) {
        dev = create_L2Device(mac_src, F);
      } else {
        dev = L2DeviceTable[mac_src];
      }
    }

    local comm: DeviceComm;
    if (mac_src !in epoch_l2_dev_comm) {
      comm = create_DeviceComm(mac_src);
    } else {
      comm = epoch_l2_dev_comm[mac_src];
    }

    local use_l2_dev_table = update_comm_table(comm, p);
    # TODO(idem) prevent mac_dst spray
    if (use_l2_dev_table && mac_dst !in L2DeviceTable) {
      create_L2Device(mac_dst, F);
    }

    if (p?$ip && p$ip$src !in dev$src_emitted_ipv4 && p$ip$src in local_nets) {
        add dev$src_emitted_ipv4[p$ip$src];
        dev$new_src_ip_ctr += 1;
    }

    if (p?$ip && mac_dst in dev$dst_emitted_ipv4 && p$ip$dst !in dev$dst_emitted_ipv4[mac_dst]) {
      local ip_dst = p$ip$dst;
      if (ip_dst in local_nets) {
        local s = dev$dst_emitted_ipv4[mac_dst];
        add s[ip_dst];

        dev$new_dst_ip_ctr += 1;
      }
    }

    if (p?$ip6 && p$ip6$src !in dev$emitted_ipv6) {
        add dev$emitted_ipv6[p$ip6$src];
    }
  }
  # TODO log non captured pkts
}

event zeek_init()
{
  local cop = create_L2Device("00:04:13", T);
  cop$label = "Copernico APs";

  local copt: table[string] of L2Device = table(["00:04:13"] = cop);
  local t = table();
  PrefixVec = [t, t, t, t, t, t, t, copt];

  print "Trying to add peer";
  Broker::peer("127.0.0.1", 9999/tcp, 0sec);
  print "Starting epoch_steps";
}

event Broker::peer_added(endpoint: Broker::EndpointInfo, msg: string)
{
    print "peer added", endpoint;
    Broker::auto_publish("monopt/l2", epoch_fire);
    Broker::auto_publish("monopt/stats", Stats::log_stats);
    schedule tick_resolution { epoch_step() };
}
