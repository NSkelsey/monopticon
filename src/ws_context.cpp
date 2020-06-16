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
    !Debug{} << "WS open:" << eventType;
    return 0;
}

static EM_BOOL WebSocketClose(int eventType, const EmscriptenWebSocketCloseEvent *e, void *userData)
{
    Context::WsBroker *b = static_cast<Context::WsBroker*>(userData);
    b->socket_connected = false;
    !Debug{} << "WS closed:" << eventType << " " << e->code << " " << e->reason;
    return 0;
}

static EM_BOOL WebSocketError(int eventType, const EmscriptenWebSocketErrorEvent *e, void *_)
{
    !Debug{} << "WS error:" << eventType << " " << e;
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
        epoch::EpochStep ee;

        ee.ParseFromIstream(&in);
        //std::cout << "new l2_size: " << ee.enter_l2devices_size() << std::endl;
        //std::cout << "device_comm: " << ee.l2_dev_comm_size() << std::endl;

        Vector2 p_1 = Util::paramCirclePoint(50, rand()%50);
        Vector3 p1 = Vector3(8*p_1.x(), 0, 8*p_1.y());
        Vector2 p_2 = Util::paramCirclePoint(50, rand()%50);
        Vector3 p2 = Vector3(8*p_2.x(), 0, 8*p_2.y());

        b->gCtx->createLines(b->sCtx, p1, p2, Util::L3Type::ARP, 1);
        b->epoch_packets_sum += 1;
    } else {
        b->tot_epoch_drop += 1;
    }

    if (b->event_cnt % 16 == 0 && b->inv_sample_rate <= 16) {
        b->inv_sample_rate = b->inv_sample_rate*2;
    }
    return 0;
}


WsBroker::WsBroker(std::string ws_uri, Graphic *g, Store *s):
    gCtx{g},
    sCtx{s}
{
    if (!emscripten_websocket_is_supported())
    {
        !Debug{} << "WebSocket creation failed, error code" << socket;
        return;
    }

    GOOGLE_PROTOBUF_VERIFY_VERSION;

    EmscriptenWebSocketCreateAttributes attr;
    emscripten_websocket_init_create_attributes(&attr);

    attr.url = ws_uri.c_str();

    socket = emscripten_websocket_new(&attr);
    if (socket <= 0)
    {
        !Debug{} << "WebSocket creation failed, error code" << socket;
        return;
    }

    int urlLength = 0;
    EMSCRIPTEN_RESULT res = emscripten_websocket_get_url_length(socket, &urlLength);
    assert(res == EMSCRIPTEN_RESULT_SUCCESS);
    assert(urlLength == strlen(url));

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