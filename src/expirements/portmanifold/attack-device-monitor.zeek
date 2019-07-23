@load base/protocols/conn/main.bro
@load policy/misc/stats.bro

global target = 192.168.1.1;

event raw_packet(p: raw_pkt_hdr)
{
    #print "rp";
}

event target_socket(sock: port)
{
    print sock;
}


event connection_SYN_packet(c: connection, pkt: SYN_packet)
{
    if (c$id$resp_h == target) {
        event target_socket(c$id$resp_p);
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
    Broker::auto_publish("monopt/port-manifold", target_socket);
    Broker::auto_publish("monopt/stats", Stats::log_stats);
}
