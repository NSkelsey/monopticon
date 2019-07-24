@load base/protocols/conn/main.bro
@load policy/misc/stats.bro

global target = 10.106.220.150;

type EpochStep: record {
  syn_summary: set[port];
  ack_summary: set[port];
};

global epoch_syn_summary: set[port];
global epoch_ack_summary: set[port];

global tick_resolution = 10msec;


event epoch_fire(m: EpochStep) {
}

event epoch_step()
{
    local msg: EpochStep;
    msg$syn_summary = epoch_syn_summary;
    msg$ack_summary = epoch_ack_summary;

    epoch_syn_summary = set();
    epoch_ack_summary = set();

    event epoch_fire(msg);

    schedule tick_resolution { epoch_step() };
}

event connection_SYN_packet(c: connection, pkt: SYN_packet)
{
    if (c$id$resp_h == target) {
        add epoch_syn_summary[c$id$resp_p];
    }
}

event connection_established(c: connection)
{
    if (c$id$resp_h == target) {
        add epoch_ack_summary[c$id$resp_p];
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
    Broker::auto_publish("monopt/epoch", epoch_fire);
    Broker::auto_publish("monopt/stats", Stats::log_stats);
    schedule tick_resolution { epoch_step() };
}
