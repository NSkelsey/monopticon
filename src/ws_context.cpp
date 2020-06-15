#include "evenbettercap.h"

namespace Monopticon::Context {

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

EM_BOOL WebSocketOpen(int eventType, const EmscriptenWebSocketOpenEvent *e, void *userData)
{
    //printf("open(eventType=%d, userData=%d)\n", eventType, (int)userData);

    epoch::L2Summary l2;
    l2.set_mac_dst(9000);
    l2.set_ipv4(100);
    int len = l2.ByteSizeLong();
    char* array = new char[len];
    l2.SerializeToArray(array, len);

    emscripten_websocket_send_binary(e->socket, array, len);
    return 0;
}

EM_BOOL WebSocketClose(int eventType, const EmscriptenWebSocketCloseEvent *e, void *userData)
{
    //printf("close(eventType=%d, wasClean=%d, code=%d, reason=%s, userData=%d)\n", eventType, e->wasClean, e->code, e->reason, (int)userData);
    return 0;
}

EM_BOOL WebSocketError(int eventType, const EmscriptenWebSocketErrorEvent *e, void *userData)
{
    //printf("error(eventType=%d, userData=%d)\n", eventType, (int)userData);
    return 0;
}

static int passed = 0;

struct Pass {
    Store* s;
    Graphic* g;
};


std::chrono::steady_clock::time_point last_msg_time;


EM_BOOL WebSocketMessage(int eventType, const EmscriptenWebSocketMessageEvent *e, void *userData)
{
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<int64_t, std::nano> dur = now - last_msg_time;

    !Debug{} << dur.count();

    Pass *p = (Pass*)userData;
    //!Debug{} << p;

    if (e->isText) {
        printf("text data: \"%s\"\n", e->data);
    }
    else {
        //printf("binary data:");
        imemstream in(e->data, e->numBytes);
        epoch::EpochStep ee;

        ee.ParseFromIstream(&in);
        //std::cout << "new l2_size: " << ee.enter_l2devices_size() << std::endl;
        //std::cout << "device_comm: " << ee.l2_dev_comm_size() << std::endl;

        Vector2 p_1 = Util::paramCirclePoint(50, rand()%50);
        Vector3 p1 = Vector3(8*p_1.x(), 0, 8*p_1.y());
        Vector2 p_2 = Util::paramCirclePoint(50, rand()%50);
        Vector3 p2 = Vector3(8*p_2.x(), 0, 8*p_2.y());

        p->g->createLines(p->s, p1, p2, Util::L3Type::ARP, 1);
    }
    return 0;
}


WsBroker::WsBroker(std::string ws_uri, Graphic *g, Store *s) {
    if (!emscripten_websocket_is_supported())
    {
        printf("WebSockets are not supported, cannot continue!\n");
        exit(1);
    }

    GOOGLE_PROTOBUF_VERIFY_VERSION;

    EmscriptenWebSocketCreateAttributes attr;
    emscripten_websocket_init_create_attributes(&attr);

    attr.url = ws_uri.c_str();

    EMSCRIPTEN_WEBSOCKET_T socket = emscripten_websocket_new(&attr);
    if (socket <= 0)
    {
        printf("WebSocket creation failed, error code %d!\n", (EMSCRIPTEN_RESULT)socket);
        exit(1);
    }

    int urlLength = 0;
    EMSCRIPTEN_RESULT res = emscripten_websocket_get_url_length(socket, &urlLength);
    assert(res == EMSCRIPTEN_RESULT_SUCCESS);
    assert(urlLength == strlen(url));

    Pass *p = new Pass{s, g};
    last_msg_time = std::chrono::steady_clock::now();

    emscripten_websocket_set_onopen_callback(socket, p, WebSocketOpen);
    emscripten_websocket_set_onclose_callback(socket, p, WebSocketClose);
    emscripten_websocket_set_onerror_callback(socket, p, WebSocketError);
    emscripten_websocket_set_onmessage_callback(socket, p, WebSocketMessage);
}

}
