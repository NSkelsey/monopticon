# Usage:
# sudo /usr/local/bro/bin/bro -b bro-peer-connector.bro
# what we want:
# /usr/local/bro/bin/bro -b bro-peer-connector.bro -r pcaps/local-sn.pcap
redef exit_only_after_terminate = T;
redef Log::enable_remote_logging = T;

global my_event: event(msg: string, c: count);

event zeek_init()
{
    print "trying to add peer";
    Broker::peer("127.0.0.1", 9999/tcp, 0sec);
    print "finished trying add peer";
}

event Broker::peer_added(endpoint: Broker::EndpointInfo, msg: string)
{
    print "peer added", endpoint;
    Broker::auto_publish("monopt/l2", raw_packet);
    #Broker::auto_publish("demo/simple", new_packet);
}

#TODO try reconnect when a peer is lost

#event new_packet(c: connection, p: pkt_hdr) {
#
#}
