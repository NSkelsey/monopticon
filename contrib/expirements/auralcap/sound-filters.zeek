module Aural;

export {
    redef enum Log::ID += { PINGLOG, PONGLOG };

    ## The log record for the traceroute log.
    type Info: record {
        ## Address initiating the traceroute.
        originator:   addr &log;
        ## Destination address of the traceroute.
        dst:   addr &log;
        ## Protocol used for the traceroute.
        proto: string &log;

        ## extra bonus!
        emitter: addr &log;
        ttl: count &log;
    };

    global log_ping: event(rec: Aural::Info);
    global log_pong: event(rec: Aural::Info);
}

redef tcp_connection_linger = 100.0secs;
redef tcp_inactivity_timeout = 100.0secs;

event zeek_init() &priority=5
{
    Log::create_stream(Aural::PONGLOG, [$columns=Info, $ev=log_ping, $path="ping"]);
    Log::create_stream(Aural::PINGLOG, [$columns=Info, $ev=log_pong, $path="pong"]);
}

event icmp_echo_request(conn: connection, icmp: icmp_conn, id: count, seq: count, payload: string) {
    print "ping";
    local tr_info : Aural::Info = [
       $originator=conn$id$orig_h,
       $dst=conn$id$resp_h,
       $proto="",
       $emitter=0.0.0.0,
       $ttl=1
    ];
    Log::write(Aural::PINGLOG, tr_info);
}

event icmp_echo_reply(conn: connection, icmp: icmp_conn, id: count, seq: count, payload: string) {
    print "pong";
    local tr_info = [
       $originator=conn$id$orig_h,
       $dst=conn$id$resp_h,
       $proto="",
       $emitter=0.0.0.0,
       $ttl=1
    ];
    Log::write(Aural::PONGLOG, tr_info);
}


# Track changes to connection states

event connection_established(conn: connection) {
  print "est";
}

event connection_reused(conn: connection) {
  print "reused";
}

event connection_rejected(conn: connection) {
  # type of failure
  print "rej";
}

event connection_reset(conn: connection) {
  # type of failure
  print "rst";
}

event connection_attempt(conn: connection) {
  print "attempt";
}

event connection_successful(conn: connection) {
  # measure connection success rates
  print "success";
}

event connection_finished(conn: connection) {
  # measure close rates
  print "fin";
}

event connection_half_finished(conn: connection) {
  print "half fin";
}

event connection_timeout(conn: connection) {
  print "conn timeout";
}

event partial_connection(conn: connection) {
  print "partial";
}

event connection_external(conn: connection) {
  print "external";
}

event tcp_mulitple_checksum_errors(conn: connection, is_orig: bool, threshold: count) {
  # type of failure
  print "tcp checksum err";
}

event tcp_multiple_zero_windows(conn: connection, is_orig: bool, threshold: count) {
  # type of failure
  print "tcp zero wins";
}

event tcp_multiple_gap(conn: connection, is_orig: bool, threshold: count) {
  # type of failure
  print "tcp multi gap";
}
