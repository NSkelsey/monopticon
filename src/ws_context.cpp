#include "evenbettercap.h"

namespace Monopticon { namespace Context {

struct membuf: std::streambuf {
    membuf(uint8_t* base, size_t size) {
        char* p(reinterpret_cast<char*>(base));
        this->setg(p, p, p + size);
    }
};

struct imemstream: virtual membuf, std::istream {
    imemstream(uint8_t * base, size_t size)
        : membuf(base, size)
        , std::istream(static_cast<std::streambuf*>(this)) {
    }
};

static EM_BOOL WebSocketOpen(int eventType, const EmscriptenWebSocketOpenEvent *e, void *userData)
{
    Context::WsBroker *b = static_cast<Context::WsBroker*>(userData);
    b->socket_connected = true;
    !Debug{} << "WS open:" << eventType << " " << e;
    return 0;
}

static EM_BOOL WebSocketClose(int eventType, const EmscriptenWebSocketCloseEvent *e, void *userData)
{
    Context::WsBroker *b = static_cast<Context::WsBroker*>(userData);
    b->socket_connected = false;
    !Debug{} << "WS closed:" << eventType << " " << e->code << " " << e->reason;
    return 0;
}

static EM_BOOL WebSocketError(int eventType, const EmscriptenWebSocketErrorEvent *e, void *userData)
{
    !Debug{} << "WS error:" << eventType << " " << e << " " << userData;
    return 0;
}


static EM_BOOL WebSocketMessage(int eventType, const EmscriptenWebSocketMessageEvent *e, void *userData)
{
    if (e->isText) {
        !Debug{} << "WS unexpected input" << eventType;
        return 0;
    }
    Context::WsBroker *b = static_cast<Context::WsBroker*>(userData);

    b->event_cnt += 1;

    if (b->event_cnt % b->inv_sample_rate == 0) {
        imemstream in(e->data, e->numBytes);
        epoch::EpochStep es;

        es.ParseFromIstream(&in);
        b->processEpochStep(es);
        
        es.Clear();
    } else {
        b->tot_epoch_drop += 1;
    }

    if (b->event_cnt % 16 == 0 && b->inv_sample_rate <= 16) {
        b->inv_sample_rate = b->inv_sample_rate*2;
    }

    return 0;
}

void WsBroker::processEpochStep(epoch::EpochStep es) {
    for (int j = 0; j < es.enter_l2devices_size(); j++) {
         const uint64_t enter_mac  = es.enter_l2devices(j);
         std::string mac_src = Util::fmtEUI48(enter_mac);

         // If the mac_src is not already in the _device_map create a new device.
         // This means that a frame with a never before seen source MAC creates a new Device.
         auto search = sCtx->_device_map.find(mac_src);
         if (search == sCtx->_device_map.end()) {
             Device::Stats *d_s = gCtx->createSphere(sCtx, mac_src);
             sCtx->_device_map.insert(std::make_pair(mac_src, d_s));
             gCtx->addDirectLabels(d_s, mac_src);
         }
    }

    int pkt_tot = 0;

    for (int i = 0; i < es.l2_dev_comm_size(); i++) {
        const epoch::DeviceComm devComm = es.l2_dev_comm(i);
        uint64_t mac = devComm.mac_src();
        std::string mac_src = Util::fmtEUI48(mac);

        Device::Stats *tran_d_s;
        auto search = sCtx->_device_map.find(mac_src);
        if (search != sCtx->_device_map.end()) {
            tran_d_s = search->second;
        } else {
            std::cerr << "tran_d_s not found! " << mac_src << std::endl;
            continue;
        }

        parse_bcast_summaries(sCtx, gCtx, devComm, tran_d_s);

        for (int k = 0; k < devComm.tx_summary_size(); k++) {
            epoch::L2Summary l2sum = devComm.tx_summary(k);
            std::string mac_dst = Util::fmtEUI48(l2sum.mac_dst());

            Device::Stats *recv_d_s;
            auto search = sCtx->_device_map.find(mac_dst);
            if (search != sCtx->_device_map.end()) {
                recv_d_s = search->second;
            } else {
                std::cerr << "recv_d_s not found! " << mac_dst << std::endl;
                continue;
            }

            Vector3 p1 = tran_d_s->circPoint;
            Vector3 p2 = recv_d_s->circPoint;

            gCtx->createLines(sCtx, p1, p2, Util::L3Type::IPV4, l2sum.ipv4());
            gCtx->createLines(sCtx, p1, p2, Util::L3Type::IPV6, l2sum.ipv6());
            gCtx->createLines(sCtx, p1, p2, Util::L3Type::ARP, l2sum.arp());
            gCtx->createLines(sCtx, p1, p2, Util::L3Type::UNKNOWN, l2sum.unknown());
            int dev_tot = Util::SumTotal(l2sum);
            tran_d_s->num_pkts_sent += dev_tot;
            recv_d_s->num_pkts_recv += dev_tot;

            pkt_tot += dev_tot;
        }
    }

    for (int m = 0; m < es.enter_l2_ipv4_addr_src_size(); m++) {
        parse_enter_l3_addr(sCtx, gCtx, es.enter_l2_ipv4_addr_src(m));
    }

    for (int p = 0; p < es.enter_arp_table_size(); p++) {
        parse_arp_table(sCtx, gCtx, es.enter_arp_table(p));
    }

    epoch_packets_sum += es.l2_dev_comm_size();
}


void WsBroker::parse_bcast_summaries(Context::Store *sCtx, Context::Graphic *gCtx, epoch::DeviceComm dComm, Device::Stats* tran_d_s) {
    parse_single_mcast(sCtx, gCtx, "33", dComm.bcast_33(), tran_d_s);
    parse_single_mcast(sCtx, gCtx, "ff", dComm.bcast_ff(), tran_d_s);
    parse_single_mcast(sCtx, gCtx, "01", dComm.bcast_01(), tran_d_s);
    parse_single_mcast(sCtx, gCtx, "odd", dComm.bcast_xx(), tran_d_s);
}


void WsBroker::parse_single_mcast(Context::Store *sCtx, Context::Graphic *gCtx, std::string v, epoch::L2Summary l2sum, Device::Stats* tran_d_s) {
    Device::PrefixStats* dp_s = sCtx->_dst_prefix_group_map.at(v);
    gCtx->createPoolHits(sCtx, tran_d_s, dp_s, l2sum);
}


void WsBroker::parse_enter_l3_addr(Context::Store *sCtx, Context::Graphic *gCtx, epoch::AddrAssoc addr_map) {
        std::string mac_src = Util::fmtEUI48(addr_map.mac_src());

        Device::Stats *tran_d_s;
        auto search = sCtx->_device_map.find(mac_src);
        if (search != sCtx->_device_map.end()) {
            tran_d_s = search->second;
        } else {
            std::cerr << "tran_d_s l2_ipv4_addr not found! " << mac_src << std::endl;
            return;
        }

        if (!tran_d_s->hasIP) {
            std::string s = Util::uint_to_ipv4addr(addr_map.ipv4());
            gCtx->createIPv4Address(sCtx, s, tran_d_s);
        }
}


void WsBroker::parse_arp_table(Context::Store *sCtx, Context::Graphic *gCtx, epoch::ArpAssoc arp_table) {
        std::string mac_src = Util::fmtEUI48(arp_table.mac_src());

        Device::Stats *tran_d_s;
        auto search = sCtx->_device_map.find(mac_src);
        if (search != sCtx->_device_map.end()) {
            tran_d_s = search->second;
        } else {
            std::cerr << "mac_src arp_table not found! " << mac_src << std::endl;
            return;
        }

        for (int j = 0; j < arp_table.table_row_size(); j++) {
            epoch::AddrAssoc row = arp_table.table_row(j);
            std::string mac_dst = Util::fmtEUI48(row.mac_src());

            Device::Stats *recv_d_s;
            auto search2 = sCtx->_device_map.find(mac_dst);
            if (search2 != sCtx->_device_map.end()) {
                recv_d_s = search2->second;
            } else {
                std::cerr << "mac_dst arp_table not found! " << mac_dst << std::endl;
                return;
            }

            // TODO validate ipv4 dest lookup against sCtx
            // int32_t ip_addr_dst = row.ipv4();

            const Vector3 offset{0.0f, 1.0f, 0.0f};
            gCtx->addL2ConnectL3(tran_d_s->circPoint, recv_d_s->circPoint+offset);
            gCtx->addL2ConnectL3(recv_d_s->circPoint+offset, tran_d_s->circPoint);
        }
}


WsBroker::WsBroker(std::string ws_uri, Graphic *g, Store *s):
    gCtx{g},
    sCtx{s}
{

    if (!emscripten_websocket_is_supported())
    {
        !Debug{} << "Will not be able to create web sockets";
        return;
    }

    GOOGLE_PROTOBUF_VERIFY_VERSION;

    openSocket(ws_uri);
}

void WsBroker::openSocket(std::string url) {
    EmscriptenWebSocketCreateAttributes attr;
    emscripten_websocket_init_create_attributes(&attr);

    attr.url = url.c_str();

    socket = emscripten_websocket_new(&attr);
    if (socket <= 0)
    {
        !Debug{} << "WebSocket creation failed, error code" << socket;
        return;
    }

    int urlLength = 0;
    EMSCRIPTEN_RESULT res = emscripten_websocket_get_url_length(socket, &urlLength);
    if (res != EMSCRIPTEN_RESULT_SUCCESS) {
        !Debug{} << "Something went wrong with the websocket";
    }

    emscripten_websocket_set_onopen_callback(socket, this, WebSocketOpen);
    emscripten_websocket_set_onclose_callback(socket, this, WebSocketClose);
    emscripten_websocket_set_onerror_callback(socket, this, WebSocketError);
    emscripten_websocket_set_onmessage_callback(socket, this, WebSocketMessage);
}

void WsBroker::closeSocket() {
    !Debug{} << "WS user close";
    emscripten_websocket_close(socket, 0, 0);
    emscripten_websocket_delete(socket);
    socket_connected = false;
}

void WsBroker::statsGui() {
    auto s = "Sample rate %.3f SPS event cnt %d";
    if (inv_sample_rate > 1.0) {
        ImGui::TextColored(ImVec4(1,0,0,1), s, 1.0/inv_sample_rate, event_cnt);
    } else {
        ImGui::Text(s, 1.0/inv_sample_rate, event_cnt);
    }

    auto r = "Pkt Lag %.1f ms; Pkt drop: %d; Epoch drop: %d";
    double t = curr_ws_lag.count()/1000000.0;
    if (curr_ws_lag > std::chrono::milliseconds(5)) {
        ImGui::TextColored(ImVec4(1,0,0,1), r, t, tot_ws_drop, tot_epoch_drop);
    } else {
        ImGui::Text(r, t, tot_ws_drop, tot_epoch_drop);
    }

    ImGui::Separator();
    ifaceChartMgr.draw();
    ImGui::Separator();
    ifaceLongChartMgr.draw();
    ImGui::Separator();
}

void WsBroker::frameUpdate() {
    event_cnt = 0;

    if (event_cnt < 8 && inv_sample_rate > 1) {
        inv_sample_rate = inv_sample_rate/2;
    }

    ifaceChartMgr.push(static_cast<float>(epoch_packets_sum));
    ifaceLongChartMgr.push(static_cast<float>(epoch_packets_sum));
    epoch_packets_sum = 0;
}

}}