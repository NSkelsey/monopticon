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
#include <Magnum/Shaders/VertexColor.h>

using namespace Magnum;
using namespace Math::Literals;

namespace Monopticon { namespace Expirements {

class SocketDrawable;

std::vector<SocketDrawable*> del_queue;

typedef Magnum::SceneGraph::Object<Magnum::SceneGraph::MatrixTransformation2D> Object2D;
typedef Magnum::SceneGraph::Scene<Magnum::SceneGraph::MatrixTransformation2D> Scene2D;

float step = 1/60.0;

class SocketDrawable: public Object2D, public SceneGraph::Drawable2D {
    public:
        explicit SocketDrawable(Object2D& object, Shaders::VertexColor2D& shader, SceneGraph::DrawableGroup2D& group, GL::Mesh &mesh):
          Object2D{&object},
          SceneGraph::Drawable2D{object, &group},
          _shader{shader},
          _mesh{mesh}
        {
          t = 1.0;
        };

        double t;

    private:
        void draw(const Matrix3& transformation, SceneGraph::Camera2D& camera) {
	    if (t > 0.0) {
		t -= step;
	    } else {
		t = 1.0;
   	    }
	    auto b = transformation*transformation.rotation(t*Rad{0.5});
            _shader.setTransformationProjectionMatrix(camera.projectionMatrix()*b);

            _mesh.draw(_shader);
        };

        Color3 _c;
        Shaders::VertexColor2D &_shader;
        GL::Mesh &_mesh;
};


class Manifold: public Platform::Application {
    public:
        explicit Manifold(const Arguments& arguments);

        Scene2D _scene;
        SceneGraph::DrawableGroup2D _drawables;
        SceneGraph::Camera2D *_camera;


        void createSocketDrawable(int port, bool ack);

    private:
        void drawEvent() override;

        GL::Mesh _mesh;
        Shaders::VertexColor2D _shader;
};

int length = 70;
int height = 25;

Manifold::Manifold(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}.setTitle("Port Manifolds")
    .setSize(Vector2i{640,480})
    }
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


    for (int y =0; y < height; y++) {
        for (int x = 0; x < length; x++) {
            createSocketDrawable(y*length + x, true);
        }
    }

    std::cout << sizeof(SocketDrawable) << std::endl;
}


void Manifold::createSocketDrawable(int port, bool ack) {
    auto const scaling = Matrix3::scaling(Vector2{0.02f});

    int x = port % length;
    int y = (port / length);

    auto *o = new Object2D{&_scene};

    Color3 c = 0xff0000_rgbf;
    if (x % 2 == 0) {
        o->rotate(180.0_degf);
     //   c = 0x0000ff_rgbf;
    }

    float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    std::cout << "initial r val:" << std::endl;
    std::cout << r << std::endl;
    o->rotate(r * 360.0_degf);

    o->transform(scaling);

    double x_off = -0.64;
    double y_off = 0.48;

    double x_v_off = 0.02;
    double y_v_off = 0.04;

    o->translate(Vector2(x_off+(x*x_v_off), y_off-(y*y_v_off)));

    auto *so = new SocketDrawable(*o, _shader, _drawables, _mesh);

    if (ack) {
        so->t = 60.0;
    }
    //all_ports.push_back(so);
}

void Manifold::drawEvent() {

    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

    _camera->draw(_drawables);

    swapBuffers();

    redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Monopticon::Expirements::Manifold)
