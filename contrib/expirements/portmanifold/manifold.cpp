#include <fstream>
#include <map>
#include <iostream>

#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Shaders/VertexColor.h>

#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/Object.h>
#include <Magnum/SceneGraph/MatrixTransformation2D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shaders/Flat.h>

#include "broker/broker.hh"
#include "broker/message.hh"
#include "broker/zeek.hh"


using namespace Magnum;
using namespace Math::Literals;

namespace Monopticon { namespace Expirements {

broker::endpoint _ep;
broker::status_subscriber _status_subscriber = _ep.make_status_subscriber(true);
broker::subscriber _subscriber = _ep.make_subscriber({"monopt/epoch", "monopt/stats"});

class SocketDrawable;

std::vector<SocketDrawable*> del_queue;

typedef Magnum::SceneGraph::Object<Magnum::SceneGraph::MatrixTransformation2D> Object2D;
typedef Magnum::SceneGraph::Scene<Magnum::SceneGraph::MatrixTransformation2D> Scene2D;

class SocketDrawable: public Object2D, public SceneGraph::Drawable2D {
    public:
        explicit SocketDrawable(Object2D& object, Shaders::Flat2D& shader, SceneGraph::DrawableGroup2D& group, Color3 c, GL::Mesh &mesh):
          Object2D{&object},
          SceneGraph::Drawable2D{object, &group},
          _c{c},
          _shader{shader},
          _mesh{mesh}
        {
          t = 1.0;
        };

        void setColor(Color3 c) {
            _c = c;
        }

        double t;

    private:
        void draw(const Matrix3& transformation, SceneGraph::Camera2D& camera) {
            if (t > 0) {
                t -= 0.08;
                _c = Color3(t, _c.g(), _c.b());
            } else {
                del_queue.push_back(this);
                return;
            }

            _shader.setColor(_c)
                   .setTransformationProjectionMatrix(camera.projectionMatrix()*transformation);

            _mesh.draw(_shader);
        };

        Color3 _c;
        Shaders::Flat2D& _shader;
        GL::Mesh &_mesh;
};


class Manifold: public Platform::Application {
    public:
        explicit Manifold(const Arguments& arguments);

        Scene2D _scene;
        SceneGraph::DrawableGroup2D _drawables;
        SceneGraph::Camera2D *_camera;

        std::map<int, SocketDrawable*> top_1000map;

        void createSocketDrawable(int port, bool ack);

    private:
        void drawEvent() override;

        GL::Mesh _mesh;
        Shaders::Flat2D _shader;
};

Manifold::Manifold(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}.setTitle("Port Manifolds")
    .setSize(Vector2i{1400,900})
    }
{
    uint16_t listen_port = 9999;
    std::string addr = "127.0.0.1";
    auto res = _ep.listen(addr, listen_port);
    if (res == 0) {
        std::cerr << "Could not listen on: ";
        std::cerr << addr << ":" << listen_port << std::endl;
        exit(1);
    } else {
        std::cout << "Endpoint listening on: ";
        std::cout << addr << ":" << listen_port << std::endl;
    }

    using namespace Math::Literals;

    Object2D *_cameraObject = new Object2D{&_scene};
    _camera = new SceneGraph::Camera2D(*_cameraObject);
    _camera->setProjectionMatrix(Matrix3::projection({4.0f/3.0f, 1.0f}))
           .setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend);

    struct TriangleVertex {
        Vector2 position;
        Color3 color;
    };

    const TriangleVertex data[]{
        {{-0.5f, -0.5f}, 0xff0000_rgbf},    /* Left vertex, red color */
        {{ 0.5f, -0.5f}, 0x00ff00_rgbf},    /* Right vertex, green color */
        {{ 0.0f,  0.5f}, 0x0000ff_rgbf}     /* Top vertex, blue color */
    };

    GL::Buffer buffer;
    buffer.setData(data);

    _mesh.setCount(3)
         .addVertexBuffer(std::move(buffer), 0,
            Shaders::VertexColor2D::Position{},
            Shaders::VertexColor2D::Color3{});


    //std::ifstream infile("../1000-ports.txt");

    // NOTE slightly less than 1000
    for (int y =0; y < 95; y++) {
        for (int x = 0; x < 650; x++) {
            //createSocketDrawable(y*650 + x);
        }
    }

    std::cout << sizeof(SocketDrawable) << std::endl;
}

void Manifold::createSocketDrawable(int port, bool ack) {
    auto const scaling = Matrix3::scaling(Vector2{0.004f});

    int x = port % 650;
    int y = (port / 650);

    auto *o = new Object2D{&_scene};

    Color3 c = 0x000000_rgbf;
    if (x % 2 == 0) {
        o->rotate(180.0_degf);
        c = 0x004444_rgbf;
    }
    o->transform(scaling);

    double x_off = -0.64;
    double y_off = 0.48;

    double x_v_off = 0.002;
    double y_v_off = 0.004;

    o->translate(Vector2(x_off+(x*x_v_off), y_off-(y*y_v_off)));

    //int p;
    //infile >> p;
    //std::cout << p << std::endl;
    auto *so = new SocketDrawable(*o, _shader, _drawables, c, _mesh);

    if (ack) {
        so->setColor(0x00ff00_rgbf);
        so->t = 60.0;
    }
    //all_ports.push_back(so);
}

void Manifold::drawEvent() {
    if (_status_subscriber.available()) {
        auto ss_res = _status_subscriber.get();

        auto err = caf::get_if<broker::error>(&ss_res);
        if (err != nullptr) {
            std::cerr << "Broker status error: " << err->code() << ", " << to_string(*err) << std::endl;
        }

        auto st = caf::get_if<broker::status>(&ss_res);
        if (st != nullptr) {
            auto ctx = st->context<broker::endpoint_info>();
            if (ctx != nullptr) {
                std::cerr << "Broker status update regarding "
                          << ctx->network->address
                          << ":" << to_string(*st) << std::endl;
            } else {
               std::cerr << "Broker status update:"
                         << to_string(*st) << std::endl;
            }
        }
    }

    for (auto msg : _subscriber.poll()) {
        broker::topic topic = broker::get_topic(msg);
        broker::zeek::Event event = broker::get_data(msg);

        auto *wrapper = broker::get_if<broker::vector>(event.args().at(0));
        if (wrapper == nullptr) {
            std::cout << "wrapper" << std::endl;
            continue;
        }


        auto *syn_summary = broker::get_if<broker::set>(wrapper->at(0));
        if (syn_summary == nullptr) {
            std::cout << "summary read failed" << std::endl;
            continue;
        }

        for (auto it = syn_summary->begin(); it != syn_summary->end(); it++) {
            broker::data p = *it;
            broker::port *syn_port = broker::get_if<broker::port>(p);
            if (syn_port == nullptr) {
                std::cerr << "syn_port" <<  p << std::endl;
                continue;
            }

            int port = syn_port->number();

            createSocketDrawable(port, false);
            //SocketDrawable *s_d = all_ports.at(port-1);
        }

        auto *ack_summary = broker::get_if<broker::set>(wrapper->at(1));
        if (ack_summary == nullptr) {
            std::cout << "ack summary read failed" << std::endl;
            continue;
        }

        if (ack_summary->size() > 0) {
            for (auto it2 = ack_summary->begin(); it2 != ack_summary->end(); it2++) {
                broker::data p = *it2;
                broker::port *ack_port = broker::get_if<broker::port>(p);
                if (ack_port == nullptr) {
                    std::cerr << "ack_port" <<  p << std::endl;
                    continue;
                }

                int port = ack_port->number();

                //createSocketDrawable(port);
                //SocketDrawable *s_d = all_ports.at(port-1);
                createSocketDrawable(port, true);
            }
        }

    }

    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

    _camera->draw(_drawables);

    for (auto it = del_queue.begin(); it != del_queue.end();) {
        SocketDrawable *pl = *it;
        delete pl;
        it = del_queue.erase(it);
    }
    //del_queue.clear();

    swapBuffers();

    redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Monopticon::Expirements::Manifold)
