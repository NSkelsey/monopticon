#include <streambuf>
#include <istream>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <emscripten/websocket.h>

#include "newproto/epoch.pb.h"

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
    printf("open(eventType=%d, userData=%d)\n", eventType, (int)userData);

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
    printf("close(eventType=%d, wasClean=%d, code=%d, reason=%s, userData=%d)\n", eventType, e->wasClean, e->code, e->reason, (int)userData);
    return 0;
}

EM_BOOL WebSocketError(int eventType, const EmscriptenWebSocketErrorEvent *e, void *userData)
{
    printf("error(eventType=%d, userData=%d)\n", eventType, (int)userData);
    return 0;
}

static int passed = 0;


EM_BOOL WebSocketMessage(int eventType, const EmscriptenWebSocketMessageEvent *e, void *userData)
{
    printf("message(eventType=%d, userData=%d, data=%p, numBytes=%d, isText=%d)\n", eventType, (int)userData, e->data, e->numBytes, e->isText);
    if (e->isText)
    {
        printf("text data: \"%s\"\n", e->data);
    }
    else
    {
        //printf("binary data:");
        imemstream in(e->data, e->numBytes);
        epoch::EpochStep ee;

        ee.ParseFromIstream(&in);
        std::cout << "new l2_size: " << ee.enter_l2devices_size() << std::endl;
        //std::cout << "device_comm: " << ee.l2_dev_comm_size() << std::endl;

        for(int i = 0; i < e->numBytes; ++i)
        {
            //printf(" %02X", e->data[i]);
        }
        //printf("\n");
    }
    return 0;
}

int main()
{
    if (!emscripten_websocket_is_supported())
    {
        printf("WebSockets are not supported, cannot continue!\n");
        exit(1);
    }

    GOOGLE_PROTOBUF_VERIFY_VERSION;

    EmscriptenWebSocketCreateAttributes attr;
    emscripten_websocket_init_create_attributes(&attr);

    const char *url = "ws://localhost:8088/";
    attr.url = url;

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

    emscripten_websocket_set_onopen_callback(socket, (void*)42, WebSocketOpen);
    emscripten_websocket_set_onclose_callback(socket, (void*)43, WebSocketClose);
    emscripten_websocket_set_onerror_callback(socket, (void*)44, WebSocketError);
    emscripten_websocket_set_onmessage_callback(socket, (void*)45, WebSocketMessage);
}
