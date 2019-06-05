# Usage:
# sudo /usr/local/bro/bin/bro -b bro-peer-connector.bro

event bro_init()
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

event new_packet(c: connection, p: pkt_hdr) {
    print "y";
}
