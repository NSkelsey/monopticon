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
broker::subscriber _subscriber = _ep.make_subscriber({"monopt/port-manifold", "monopt/stats"});


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
        {};

        void setColor(Color3 c) {
            _c = c;
        }

    private:
        void draw(const Matrix3& transformation, SceneGraph::Camera2D& camera) {

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

    auto scaling = Matrix3::scaling(Vector2{0.04f});

    std::ifstream infile("../1000-ports.txt");

    // NOTE slightly less than 1000
    for (int y =0; y < 15; y++) {
        for (int x = 0; x < 65; x++) {
            auto *o = new Object2D{&_scene};

            Color3 c = 0xffffff_rgbf;
            if (x % 2 == 0) {
                o->rotate(180.0_degf);
                c = 0xeeeeee_rgbf;
            }
            o->transform(scaling);

            double x_off = -0.64;
            double y_off = 0.48;

            double x_v_off = 0.02;
            double y_v_off = 0.04;

            o->translate(Vector2(x_off+(x*x_v_off), y_off-(y*y_v_off)));

            int p;
            infile >> p;
            //std::cout << p << std::endl;
            auto *so = new SocketDrawable(*o, _shader, _drawables, c, _mesh);
            top_1000map.insert(std::make_pair(p, so));
        }
    }
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
        broker::port *syn_port = broker::get_if<broker::port>(event.args().at(0));
        if (syn_port == nullptr) {
            std::cerr << "syn_port" <<  std::endl;
            continue;
        }
        std::cout << *syn_port << std::endl;

        int port = syn_port->number();

        SocketDrawable *s_d;
        auto search = top_1000map.find(port);
        if (search != top_1000map.end()) {
            s_d = search->second;
            s_d->setColor(0xff0000_rgbf);
        } else {
            std::cerr << "socket drawable not found! " << port << std::endl;
            continue;
        }
    }

    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

    _camera->draw(_drawables);

    swapBuffers();

    redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Monopticon::Expirements::Manifold)
