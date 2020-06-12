#include <fstream>
#include <map>
#include <iostream>

#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Shaders/VertexColor.h>
#include <Magnum/Animation/Track.h>

#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/Object.h>
#include <Magnum/SceneGraph/MatrixTransformation2D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shaders/Flat.h>

using namespace Magnum;
using namespace Math::Literals;

namespace Monopticon { namespace Expirements {

class SocketDrawable;

std::vector<SocketDrawable*> del_queue;

struct Keyframe {
    Float time;
    Vector2 position;
};

const Keyframe data[] {
    {0.0f, Vector2::yAxis(0.0f)},
    {1.0f, Vector2::yAxis(0.5f)},
    {2.0f, Vector2::yAxis(0.75f)},
    {3.0f, Vector2::yAxis(0.875f)},
    {4.0f, Vector2::yAxis(0.75f)},
    {5.0f, Vector2::yAxis(0.5f)},
    {6.0f, Vector2::yAxis(0.0f)}
};


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

            Animation::TrackView<const Float, const Vector2> positions {
                {data, &data[0].time, Containers::arraySize(data), sizeof(Keyframe)},
                {data, &data[0].position, Containers::arraySize(data), sizeof(Keyframe)},
                Math::lerp
            };

            positions.at(2.2);
        };


        void setColor(Color3 c) {
            _c = c;
        }

        double t;

    private:
        void draw(const Matrix3& transformation, SceneGraph::Camera2D& camera) {
            if (t > 0) {
                t -= 0.001;
                _c = Color3(t, _c.g(), _c.b());
            } else {
                del_queue.push_back(this);
                return;
            }

            _shader.setColor(_c)
                   .setTransformationProjectionMatrix(camera.projectionMatrix()*transformation);

            _shader.draw(_mesh);
        };

        Color3 _c;
        Shaders::Flat2D& _shader;
        GL::Mesh &_mesh;
};


class Manifold: public Platform::Application {
    public:
        explicit Manifold(const Arguments& arguments);

        Scene2D _scene; SceneGraph::DrawableGroup2D _drawables; SceneGraph::Camera2D *_camera;
       
        std::map<int, SocketDrawable*> top_1000map;

        void createSocketDrawable(int port, bool ack);

    private:
        void drawEvent() override;

        GL::Mesh _mesh;
        Shaders::Flat2D _shader;
};

Manifold::Manifold(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}
        .setTitle("Animation Test")
        .setSize(Vector2i{1400,900}),
        GLConfiguration{}.setSampleCount(4)}
{
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
            if (x%100 == 0 && y%10 == 0) {
                createSocketDrawable(y*650 + x, false);
            }
        }
    }

    std::cout << sizeof(SocketDrawable) << std::endl;
}

void Manifold::createSocketDrawable(int port, bool ack) {
    auto const scaling = Matrix3::scaling(Vector2{0.016f});

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

    auto *so = new SocketDrawable(*o, _shader, _drawables, c, _mesh);

    if (ack) {
        so->setColor(0x00ff00_rgbf);
        so->t = 60.0;
    }
}

void Manifold::drawEvent() {
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