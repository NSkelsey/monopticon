#include <stdio.h>
#include <iostream>
#include <math.h>
#include <ctime>
#include <set>

#include <imgui.h>

#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Pointer.h>
#include <Corrade/Containers/Reference.h>
#include <Corrade/Utility/Resource.h>

#include <Magnum/Timeline.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Image.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Constants.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/MeshTools/Transform.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Primitives/Circle.h>
#include <Magnum/Primitives/Line.h>
#include <Magnum/Primitives/UVSphere.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shaders/Flat.h>
#include <Magnum/Shaders/MeshVisualizer.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Trade/MeshData3D.h>

#include "broker/broker.hh"
#include "broker/message.hh"
#include "broker/bro.hh"

#include <unordered_map>

// Zeek broker components
broker::endpoint ep;
broker::subscriber subscriber = ep.make_subscriber({"monopt/l2"});

namespace Magnum { namespace Monopticon {

void parse_raw_packet(broker::bro::Event event);

using namespace Math::Literals;

typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D> Object3D;
typedef SceneGraph::Scene<SceneGraph::MatrixTransformation3D> Scene3D;

Vector2 randCirclePoint();
Vector2 randOffset(float z);

class DeviceStats;


class PhongIdShader: public GL::AbstractShaderProgram {
    public:
        typedef GL::Attribute<0, Vector3> Position;
        typedef GL::Attribute<1, Vector3> Normal;

        enum: UnsignedInt {
            ColorOutput = 0,
            ObjectIdOutput = 1
        };

        explicit PhongIdShader();

        PhongIdShader& setObjectId(UnsignedInt id) {
            setUniform(_objectIdUniform, id);
            return *this;
        }

        PhongIdShader& setLightPosition(const Vector3& position) {
            setUniform(_lightPositionUniform, position);
            return *this;
        }

        PhongIdShader& setAmbientColor(const Color3& color) {
            setUniform(_ambientColorUniform, color);
            return *this;
        }

        PhongIdShader& setColor(const Color3& color) {
            setUniform(_colorUniform, color);
            return *this;
        }

        PhongIdShader& setTimeIntensity(const float t) {
            setUniform(_timeIntensityUniform, t);
            return *this;
        }

        PhongIdShader& setTransformationMatrix(const Matrix4& matrix) {
            setUniform(_transformationMatrixUniform, matrix);
            return *this;
        }

        PhongIdShader& setNormalMatrix(const Matrix3x3& matrix) {
            setUniform(_normalMatrixUniform, matrix);
            return *this;
        }

        PhongIdShader& setProjectionMatrix(const Matrix4& matrix) {
            setUniform(_projectionMatrixUniform, matrix);
            return *this;
        }

    private:
        Int _objectIdUniform,
            _lightPositionUniform,
            _ambientColorUniform,
            _colorUniform,
            _timeIntensityUniform,
            _transformationMatrixUniform,
            _normalMatrixUniform,
            _projectionMatrixUniform;
};

PhongIdShader::PhongIdShader() {
    Utility::Resource rs("picking-data");

    GL::Shader vert{GL::Version::GL330, GL::Shader::Type::Vertex},
        frag{GL::Version::GL330, GL::Shader::Type::Fragment};
    vert.addSource(rs.get("shaders/phongid.vs"));
    frag.addSource(rs.get("shaders/phongid.fs"));
    CORRADE_INTERNAL_ASSERT(GL::Shader::compile({vert, frag}));
    attachShaders({vert, frag});
    CORRADE_INTERNAL_ASSERT(link());

    _objectIdUniform = uniformLocation("objectId");
    _lightPositionUniform = uniformLocation("light");
    _ambientColorUniform = uniformLocation("ambientColor");
    _colorUniform = uniformLocation("color");
    _timeIntensityUniform = uniformLocation("timeIntensity");
    _transformationMatrixUniform = uniformLocation("transformationMatrix");
    _projectionMatrixUniform = uniformLocation("projectionMatrix");
    _normalMatrixUniform = uniformLocation("normalMatrix");
}


class DeviceDrawable: public SceneGraph::Drawable3D {
    public:
        explicit DeviceDrawable(UnsignedByte id, Object3D& object, PhongIdShader& shader, Color3 &color, GL::Mesh& mesh, const Matrix4& primitiveTransformation, SceneGraph::DrawableGroup3D& drawables):
            SceneGraph::Drawable3D{object, &drawables},
            _id{id},
            _selected{false},
            _color{color},
            _shader(shader),
            _mesh(mesh),
            _primitiveTransformation{primitiveTransformation} {
              _t = 0.0f;
            }

        void setSelected(bool selected) {
            _selected = selected;
            if (selected) _t = 1.0f;
        }

        bool isSelected() {
            return _selected;
        }

    private:
        void draw(const Matrix4& transformation, SceneGraph::Camera3D& camera) override {
            if (_t > 0.0f) {
                _t = _t - 0.01f;
            }

            _shader.setTransformationMatrix(transformation*_primitiveTransformation)
                   .setNormalMatrix(transformation.rotationScaling())
                   .setProjectionMatrix(camera.projectionMatrix())
                   .setAmbientColor(_selected ? _color*0.2f : Color3{})
                   .setColor(_color*(_selected ? 1.5f : 0.9f))
                   .setTimeIntensity(_t)
                   /* relative to the camera */
                   .setLightPosition({0.0f, 4.0f, 3.0f})
                   .setObjectId(_id);
            _mesh.draw(_shader);
        }

        UnsignedByte _id;
        bool _selected;
        Color3 _color;
        PhongIdShader& _shader;
        GL::Mesh& _mesh;
        Matrix4 _primitiveTransformation;
        float _t;
};


class RingDrawable: public SceneGraph::Drawable3D {
    public:
        explicit RingDrawable(Object3D& object, const Color4& color, SceneGraph::DrawableGroup3D& group):
            SceneGraph::Drawable3D{object, &group}
        {

            _mesh = MeshTools::compile(Primitives::circle3DWireframe(70));
            _color = color;
            _shader = Shaders::MeshVisualizer{Shaders::MeshVisualizer::Flag::Wireframe|Shaders::MeshVisualizer::Flag::NoGeometryShader};
        }

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override {
            using namespace Math::Literals;

            _shader.setColor(0xffffff_rgbf)
                   .setWireframeColor(_color)
                   .setTransformationProjectionMatrix(camera.projectionMatrix()*transformationMatrix);
            _mesh.draw(_shader);
        }

        Matrix4 scaling = Matrix4::scaling(Vector3{10});

        GL::Mesh _mesh;
        Color4 _color;
        Shaders::MeshVisualizer _shader;
};


class ParaLineShader: public GL::AbstractShaderProgram {
    public:
        typedef GL::Attribute<0, Vector3> Position;

        explicit ParaLineShader();

        ParaLineShader& setColor(const Color3& color) {
            setUniform(_colorUniform, color);
            return *this;
        }

        ParaLineShader& setBPos(const Vector3& position) {
            setUniform(_bPosUniform, position);
            return *this;
        }

        ParaLineShader& setTParam(const float t) {
            setUniform(_tParamUniform, t);
            return *this;
        }

        ParaLineShader& setTransformationProjectionMatrix(const Matrix4& matrix) {
            setUniform(_transformationProjectionMatrixUniform, matrix);
            return *this;
        }

    private:
        Int _colorUniform,
            _bPosUniform,
            _tParamUniform,
            _transformationProjectionMatrixUniform;

};

ParaLineShader::ParaLineShader() {
    Utility::Resource rs("picking-data");

    GL::Shader vert{GL::Version::GL330, GL::Shader::Type::Vertex},
        frag{GL::Version::GL330, GL::Shader::Type::Fragment};
    vert.addSource(rs.get("shaders/packetline.vs"));
    frag.addSource(rs.get("shaders/packetline.fs"));
    CORRADE_INTERNAL_ASSERT(GL::Shader::compile({vert, frag}));
    attachShaders({vert, frag});
    CORRADE_INTERNAL_ASSERT(link());

    _bPosUniform = uniformLocation("bPos");
    _colorUniform = uniformLocation("color");
    _tParamUniform = uniformLocation("tParam");
    _transformationProjectionMatrixUniform = uniformLocation("transformationProjectionMatrix");

    //CORRADE_INTERNAL_ASSERT(_bPosUniform >= 0);
}


class PacketLineDrawable: public SceneGraph::Drawable3D {
    public:
        explicit PacketLineDrawable(Object3D& object, ParaLineShader& shader, Vector3& a, Vector3& b, SceneGraph::DrawableGroup3D& group):
            SceneGraph::Drawable3D{object, &group},
            _object{object},
            _shader{shader},
            _b{b}
        {
            _t = 1.0f;
            _mesh = MeshTools::compile(Primitives::line3D(a,b));
            _expired = false;
        }

    Object3D &_object;
    bool _expired;

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override {
            if (_t < 1.001f && _t > 0.0f) {
                _t -= 0.02f;
                Matrix4 scaling = Matrix4::scaling(Vector3{_t});
                _object.transform(scaling);
            }
            if (_t < 0.0f) {
                _expired=true;
                return;
            }

            _shader.setTransformationProjectionMatrix(camera.projectionMatrix()*transformationMatrix)
                   .setBPos(_b)
                   .setTParam(_t);
            _mesh.draw(_shader);
        }

        GL::Mesh _mesh;
        ParaLineShader& _shader;
        Vector3 _b;
        float _t;
};


class DeviceStats {
  public:
     DeviceStats(std::string macAddr, Vector2 pos, DeviceDrawable *dev):
         mac_addr{macAddr},
         circPoint{pos},
         _drawable{dev}
     {};

     std::string create_device_string() {
         std::ostringstream stringStream;
         stringStream << this->mac_addr;
         stringStream << " | ";
         stringStream << this->num_pkts_sent;
         stringStream << " | ";
         stringStream << this->num_pkts_recv;
         std::string c = stringStream.str();
         return c;
     };

     std::string mac_addr;
     Vector2 circPoint;
     int num_pkts_sent = 0;
     int num_pkts_recv = 0;
     DeviceDrawable *_drawable;
};


class ObjSelect: public Platform::Application {
    public:
        explicit ObjSelect(const Arguments& arguments);

        void drawEvent() override;

        void viewportEvent(ViewportEvent& event) override;

        void keyPressEvent(KeyEvent& event) override;
        void keyReleaseEvent(KeyEvent& event) override;

        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;
        void mouseScrollEvent(MouseScrollEvent& event) override;
        void textInputEvent(TextInputEvent& event) override;

        void parse_raw_packet(broker::bro::Event event);
        DeviceStats* createCircle(const std::string);
        void createLine(Vector2, Vector2);
        void deselectAllDevices();

    private:
        // UI fields
        ImGuiIntegration::Context _imgui{NoCreate};

        // Graphic fields
        GL::Mesh _sphere, _circle{NoCreate};
        Color4 _clearColor = 0x002b36_rgbf;
        Color3 _pickColor = 0xffffff_rgbf;

        GL::Buffer _indexBuffer, _vertexBuffer;
        PhongIdShader _phong_id_shader;

        ParaLineShader _line_shader;

        Scene3D _scene;
        SceneGraph::Camera3D* _camera;
        SceneGraph::DrawableGroup3D _drawables;
        Timeline _timeline;

        Object3D *_cameraRig, *_cameraObject;

        GL::Framebuffer _framebuffer;
        Vector2i _previousMousePosition, _mousePressPosition;
        GL::Renderbuffer _color, _objectId, _depth;

        GL::Buffer _sphereVertices, _sphereIndices;

        std::vector<DeviceDrawable*> _device_objects{};
        std::set<PacketLineDrawable*> _packet_line_queue{};
        std::unordered_map<std::string, DeviceStats> _device_map{};

        bool _drawCubes{true};
};


ObjSelect::ObjSelect(const Arguments& arguments):
        Platform::Application{arguments, Configuration{}
            .setTitle("Monopticon")
            .setWindowFlags(Configuration::WindowFlag::Borderless|Configuration::WindowFlag::Resizable)
            .setSize(Vector2i{1200,800}),
            GLConfiguration{}.setSampleCount(8)},
        _framebuffer{GL::defaultFramebuffer.viewport()}
{
    std::cout << "Waiting for broker connection" << std::endl;

    ep.listen("127.0.0.1", 9999);

    std::cout << "Connection received" << std::endl;

    /* Global renderer configuration */
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);

    _color.setStorage(GL::RenderbufferFormat::RGBA8, GL::defaultFramebuffer.viewport().size());
    _objectId.setStorage(GL::RenderbufferFormat::R32UI, GL::defaultFramebuffer.viewport().size());
    _depth.setStorage(GL::RenderbufferFormat::DepthComponent24, GL::defaultFramebuffer.viewport().size());
    _framebuffer.attachRenderbuffer(GL::Framebuffer::ColorAttachment{0}, _color)
                .attachRenderbuffer(GL::Framebuffer::ColorAttachment{1}, _objectId)
                .attachRenderbuffer(GL::Framebuffer::BufferAttachment::Depth, _depth)
                .mapForDraw({{PhongIdShader::ColorOutput, GL::Framebuffer::ColorAttachment{0}},
                            {PhongIdShader::ObjectIdOutput, GL::Framebuffer::ColorAttachment{1}}});
    CORRADE_INTERNAL_ASSERT(_framebuffer.checkStatus(GL::FramebufferTarget::Draw) == GL::Framebuffer::Status::Complete);

    /* Camera setup */
    (*(_cameraRig = new Object3D{&_scene}))
        //.translate(Vector3::yAxis(3.0f))
        .rotateY(40.0_degf);
    (*(_cameraObject = new Object3D{_cameraRig}))
        .translate(Vector3::zAxis(30.0f))
        .rotateX(-25.0_degf);
    (_camera = new SceneGraph::Camera3D(*_cameraObject))
        ->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(40.0_degf, 1.0f, 0.001f, 100.0f))
        .setViewport(GL::defaultFramebuffer.viewport().size()); /* Drawing setup */


    {
        Trade::MeshData3D data = Primitives::uvSphereSolid(8.0f, 30.0f);
        _sphereVertices.setData(MeshTools::interleave(data.positions(0), data.normals(0)), GL::BufferUsage::StaticDraw);
        _sphereIndices.setData(MeshTools::compressIndicesAs<UnsignedShort>(data.indices()), GL::BufferUsage::StaticDraw);
        _sphere.setCount(data.indices().size())
            .setPrimitive(data.primitive())
            .addVertexBuffer(_sphereVertices, 0, PhongIdShader::Position{}, PhongIdShader::Normal{})
            .setIndexBuffer(_sphereIndices, 0, MeshIndexType::UnsignedShort);
    }

    _line_shader = ParaLineShader{};
    _line_shader.setColor(0x00ffff_rgbf);
    _phong_id_shader = PhongIdShader{};

    srand(time(NULL));


    { // Known devices
        Object3D *cob = new Object3D{&_scene};
        Matrix4 scaling = Matrix4::scaling(Vector3{4.8});
        cob->transform(scaling);
        cob->rotateX(90.0_degf);
        cob->translate(Vector3{5.0f, 0.0f, 5.0f});
        new RingDrawable{*cob, 0x0000ff_rgbf, _drawables};
    }
    { // Gateways
        Object3D *coy = new Object3D{&_scene};
        Matrix4 scaling = Matrix4::scaling(Vector3{4.8});
        coy->transform(scaling);
        coy->rotateX(90.0_degf);
        coy->translate(Vector3{5.0f, 0.0f, -5.0f});
        new RingDrawable{*coy, 0xffff00_rgbf, _drawables};
    }
    { // Unknown devices
        Object3D *cor = new Object3D{&_scene};
        Matrix4 scaling = Matrix4::scaling(Vector3{4.8});
        cor->rotateX(90.0_degf);
        cor->transform(scaling);
        cor->translate(Vector3{-5.0f, 0.0f, -5.0f});
        new RingDrawable{*cor, 0xff0000_rgbf, _drawables};
    }
    { // Broadcast addrs
        Object3D *cog = new Object3D{&_scene};
        Matrix4 scaling = Matrix4::scaling(Vector3{4.8});
        cog->rotateX(90.0_degf);
        cog->transform(scaling);
        cog->translate(Vector3{-5.0f, 0.0f, 5.0f});
        new RingDrawable{*cog, 0x00ff00_rgbf, _drawables};
    }

    Vector2 p1, p2;
    {
        std::string mac_dst = "ba:dd:be:ee:ef";
        DeviceStats *d_s = createCircle(mac_dst);
        p1 = d_s->circPoint;
    }
    {
        std::string mac_dst = "ca:ff:eb:ee:ef";
        DeviceStats *d_s = createCircle(mac_dst);
        p2 = d_s->circPoint;
    }

    createLine(p1, p2);

    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::enable(GL::Renderer::Feature::PolygonOffsetFill);
    GL::Renderer::setPolygonOffset(2.0f, 0.5f);

    _imgui = ImGuiIntegration::Context(Vector2{windowSize()}/dpiScaling(),
        windowSize(), framebufferSize());

    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
        GL::Renderer::BlendEquation::Add);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

    setSwapInterval(1);
    setMinimalLoopPeriod(16);
    _timeline.start();
}


void ObjSelect::parse_raw_packet(broker::bro::Event event) {
    broker::vector parent_content = event.args();

    broker::vector *raw_pkt_hdr = broker::get_if<broker::vector>(parent_content.at(0));
    if (raw_pkt_hdr == NULL) {
        std::cout << "rph" << std::endl;
        return;
    }
    broker::vector *l2_pkt_hdr = broker::get_if<broker::vector>(raw_pkt_hdr->at(0));
    if (l2_pkt_hdr == NULL || l2_pkt_hdr->size() != 9) {
        std::cout << "lph" << std::endl;
        return;
    }

    std::string *mac_src = broker::get_if<std::string>(l2_pkt_hdr->at(3));
    if (mac_src == NULL) {
        std::cout << "mac_src null" << std::endl;
        return;
    }
    std::string *mac_dst = broker::get_if<std::string>(l2_pkt_hdr->at(4));
    if (mac_dst == NULL) {
        std::cout << "mac_dst null" << std::endl;
        return;
    }

    DeviceStats *d_s;

    auto search = _device_map.find(*mac_src);
    if (search == _device_map.end()) {
        d_s = createCircle(*mac_src);
        _device_map.insert(std::make_pair(*mac_src, *d_s));
    } else {
        d_s = &(search->second);
    }
    d_s->num_pkts_sent += 1;
    Vector2 p1 = d_s->circPoint;

    auto search_dst = _device_map.find(*mac_dst);
    if (search_dst == _device_map.end()) {
        d_s = createCircle(*mac_dst);
    } else {
        d_s = &(search_dst->second);
    }
    d_s->num_pkts_recv += 1;
    Vector2 p2 = d_s->circPoint;

    createLine(p1, p2);
}

void ObjSelect::deselectAllDevices() {
    /* Highlight object under mouse and deselect all other */
    for(std::vector<DeviceDrawable*>::iterator it = _device_objects.begin(); it != _device_objects.end(); ++it) {
        (*it)->setSelected(false);
    }
}

DeviceStats* ObjSelect::createCircle(const std::string mac) {
    Object3D* o = new Object3D{&_scene};

    Vector2 v = 4.0f*randCirclePoint();
    Vector2 z = randOffset(5.0f);
    v = v + z;

    o->translate({v.x(), 0.0f, v.y()});

    UnsignedByte id = static_cast<UnsignedByte>(_device_objects.size()+1);

    Color3 c = 0xa5c9ea_rgbf;
    DeviceDrawable *dev = new DeviceDrawable{id, *o, _phong_id_shader, c, _sphere,
        Matrix4::scaling(Vector3{0.25f}), _drawables};

    _device_objects.push_back(dev);

    DeviceStats* d_s = new DeviceStats{mac, v, dev};

    _device_map.insert(std::make_pair(mac, *d_s));

    return d_s;
}


void ObjSelect::createLine(Vector2 a, Vector2 b) {
        Object3D* line = new Object3D{&_scene};
        Vector3 a3 = Vector3{a.x(), 0.0f, a.y()};
        Vector3 b3 = Vector3{b.x(), 0.0f, b.y()};

        auto *pl = new PacketLineDrawable{*line, _line_shader, a3, b3, _drawables};
        _packet_line_queue.insert(pl);
}


void ObjSelect::drawEvent() {
    for (auto msg : subscriber.poll()) {
        broker::topic topic = broker::get_topic(msg);
        broker::bro::Event event = broker::get_data(msg);
        std::cout << "received on topic: " << topic << " event: " << event.args() << std::endl;
        if (event.name().compare("raw_packet_event")) {
            parse_raw_packet(event);
        } else {
            std::cout << "Unhandled Event: " << event.name() << std::endl;
        }
    }

    // TODO TODO TODO TODO
    // Expire devices that haven't communicated in awhile
    // TODO TODO TODO TODO
    // steps
    // 0: add shader uniform to expire device
    // 1: refactor _device id vector to map of ids
    // 2: create _expired member and update phongid draw
    // 3: remove expired devices

    std::set<PacketLineDrawable *>::iterator it;
    for (it = _packet_line_queue.begin(); it != _packet_line_queue.end(); ) {
        PacketLineDrawable *pl = *it;
        if (pl->_expired) {
            it = _packet_line_queue.erase(it);
            delete pl;
        } else {
            ++it;
        }
    }

    // Actually draw things

    /* Rotate the camera on an orbit */
    //_cameraRig->rotateY(0.10_degf);

    /* Draw to custom framebuffer */
    _framebuffer
        .clearColor(0, _clearColor)
        .clearColor(1, Vector4ui{})
        .clearDepth(1.0f)
        .bind();
    _camera->draw(_drawables);


    /* Bind the main buffer back */
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth)
        .bind();

    GL::Renderer::setClearColor(_clearColor);

    /* Blit color to window framebuffer */
    _framebuffer.mapForRead(GL::Framebuffer::ColorAttachment{0});
    GL::AbstractFramebuffer::blit(_framebuffer, GL::defaultFramebuffer,
        {{}, _framebuffer.viewport().size()}, GL::FramebufferBlit::Color);

    _imgui.newFrame();

    ImGui::SetNextWindowSize(ImVec2(300, 210), ImGuiSetCond_Always);
    ImGui::Begin("Tap Status");

    if (ImGui::Button("Disconnect", ImVec2(80, 20))) {
    }

    const int len_v = 60;
    static float values[len_v] = { 0 };
    static float values2[len_v] = { 0 };
    static int values_offset = 0;
    static double refresh_time = 0.0;
    float tot = 0;
    if (refresh_time == 0.0)
        refresh_time = ImGui::GetTime();
    while (refresh_time < ImGui::GetTime()) // Create dummy data at fixed 60 hz rate for the demo
    {
        float x = (float)(1 + std::rand()/((RAND_MAX + 1u)/2));
        int y = 1 + std::rand()/((RAND_MAX + 1u)/2);
        tot = tot + x;
        values[values_offset] = x;
        values2[values_offset] = (float)y;
        values_offset = (values_offset+1) % len_v;
        refresh_time += 4.0f/60.0f;
    }

    ImGui::Separator();
    char c[40];
    ImGui::Text("App average %.3f ms/frame (%.1f FPS)",
        1000.0/Double(ImGui::GetIO().Framerate), Double(ImGui::GetIO().Framerate));
    sprintf(c, "avg tx ppf %0.6f", tot/len_v);
    ImGui::PlotLines("", values, len_v, values_offset, c, 0.0f, 4.0f, ImVec2(300,60));
    sprintf(c, "avg rx ppf %0.6f", tot/len_v);
    ImGui::PlotLines("", values2, len_v, values_offset, c, 0.0f, 4.0f, ImVec2(300,60));
    ImGui::Separator();
    ImGui::End();

    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiSetCond_Once);
    ImGui::Begin("Heads Up Display");
    ImGui::Text("Observed Addresses");
    ImGui::Separator();
    ImGui::BeginChild("Scrolling");
    for (auto it = _device_map.begin(); it != _device_map.end(); it++) {
        DeviceStats *d_s = &(it->second);
        std::string s = d_s->create_device_string();

        if (d_s->_drawable->isSelected()) {
            ImGui::TextColored(ImVec4(1,1,0,1), "%s", s.c_str());
        } else {
            ImGui::Text("%s", s.c_str());
        }
        /*
        ImVec2 dimensions = ImVec2(300,20);
        if (ImGui::Button("xxx", dimensions)) {
            deselectAllDevices();
            d_s->_drawable->setSelected(true);
        }*/
    }
    ImGui::EndChild();
    ImGui::End();

    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);

    _imgui.drawFrame();

    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);

    swapBuffers();
    _timeline.nextFrame();
    redraw();
}


void ObjSelect::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});

    _imgui.relayout(Vector2{event.windowSize()}/event.dpiScaling(),
        event.windowSize(), event.framebufferSize());
}


void ObjSelect::keyPressEvent(KeyEvent& event) {
    if(_imgui.handleKeyPressEvent(event)) return;

    /* Movement */
    if(event.key() == KeyEvent::Key::Down) {
        _cameraObject->rotateX(5.0_degf);
    } else if(event.key() == KeyEvent::Key::Up) {
        _cameraObject->rotateX(-5.0_degf);
    } else if(event.key() == KeyEvent::Key::Left) {
        _cameraRig->rotateY(-5.0_degf);
    } else if(event.key() == KeyEvent::Key::Right) {
        _cameraRig->rotateY(5.0_degf);
    }
}


void ObjSelect::keyReleaseEvent(KeyEvent& event) {
    if(_imgui.handleKeyReleaseEvent(event)) return;
}


void ObjSelect::mousePressEvent(MouseEvent& event) {
    if(_imgui.handleMousePressEvent(event)) return;

    if(event.button() != MouseEvent::Button::Left) return;

    _previousMousePosition = _mousePressPosition = event.position();

    event.setAccepted();
}

void ObjSelect::mouseReleaseEvent(MouseEvent& event) {
    if(_imgui.handleMouseReleaseEvent(event)) return;

    if(event.button() != MouseEvent::Button::Left || _mousePressPosition != event.position()) return;

    /* Read object ID at given click position (framebuffer has Y up while windowing system Y down) */
    _framebuffer.mapForRead(GL::Framebuffer::ColorAttachment{1});
    Image2D data = _framebuffer.read(
        Range2Di::fromSize({event.position().x(), _framebuffer.viewport().sizeY() - event.position().y() - 1}, {1, 1}),
        {PixelFormat::R32UI});


    deselectAllDevices();

    UnsignedByte id = data.data<UnsignedByte>()[0];
    if(id > 0 && id < _device_objects.size()+1) {
        _device_objects.at(id-1)->setSelected(true);
    }

    event.setAccepted();
    redraw();
}


void ObjSelect::mouseMoveEvent(MouseMoveEvent& event) {
    if(_imgui.handleMouseMoveEvent(event)) return;

    if(!(event.buttons() & MouseMoveEvent::Button::Left)) return;

    const Vector2 delta = 3.0f*
        Vector2{event.position() - _previousMousePosition}/
        Vector2{GL::defaultFramebuffer.viewport().size()};

    (*_cameraObject)
        .rotate(Rad{-delta.y()}, _cameraObject->transformation().right().normalized())
        .rotateY(Rad{-delta.x()});

    _previousMousePosition = event.position();
    event.setAccepted();
    redraw();
}

void ObjSelect::mouseScrollEvent(MouseScrollEvent& event) {
    if(_imgui.handleMouseScrollEvent(event)) return;
}

void ObjSelect::textInputEvent(TextInputEvent& event) {
    if(_imgui.handleTextInputEvent(event)) return;
}


Vector2 randCirclePoint() {
    float f =  rand() / (RAND_MAX/(2*Math::Constants<float>::pi()));

    return Vector2{cos(f), sin(f)};
}

Vector2 randOffset(float z) {
    int x = rand() % 2;
    int y = rand() % 2;
    Vector2 v = Vector2{x ? z : -z, y ? z : -z};
    return v;
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Monopticon::ObjSelect)
