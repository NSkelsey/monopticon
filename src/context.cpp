#include "evenbettercap.h"

using namespace Monopticon::Context;

GraphicsCtx::GraphicsCtx():
    _glyphCache(Vector2i(2048), Vector2i(512), 22)
{
    MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL330);

    auto viewport = GL::defaultFramebuffer.viewport();
    prepareGLBuffers(viewport);

    /* Camera setup */
    (*(_cameraRig = new Object3D{&_scene}))
        .rotateY(40.0_degf);
    (*(_cameraObject = new Object3D{_cameraRig}))
        .translate(Vector3::zAxis(20.0f))
        .rotateX(-25.0_degf);
    (_camera = new SceneGraph::Camera3D(*_cameraObject))
        ->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(50.0_degf, 1.0f, 0.001f, 100.0f))
        .setViewport(viewport.size());


    _poolCircle = MeshTools::compile(Primitives::circle3DWireframe(20));

    _line_shader = Figure::ParaLineShader{};
    _phong_id_shader = Figure::PhongIdShader{};
    _pool_shader = Figure::PoolShader{};
    _link_shader = Figure::WorldLinkShader{};

    _bbitem_shader = Shaders::Flat3D{};
    _bbitem_shader.setColor(0x00ff00_rgbf);

    {
        Trade::MeshData3D data = Primitives::uvSphereSolid(8.0f, 30.0f);

        GL::Buffer sphereVertices, sphereIndices;

        sphereVertices.setData(MeshTools::interleave(data.positions(0), data.normals(0)), GL::BufferUsage::StaticDraw);
        sphereIndices.setData(MeshTools::compressIndicesAs<UnsignedShort>(data.indices()), GL::BufferUsage::StaticDraw);
        _sphere.setCount(data.indices().size())
               .setPrimitive(data.primitive())
               .addVertexBuffer(sphereVertices, 0, Figure::PhongIdShader::Position{}, Figure::PhongIdShader::Normal{})
               .setIndexBuffer(sphereIndices, 0, MeshIndexType::UnsignedShort);
    }
    {
        Trade::MeshData3D data = Primitives::cubeSolid();

        GL::Buffer cubeVertices, cubeIndices;

        cubeVertices.setData(MeshTools::interleave(data.positions(0), data.normals(0)), GL::BufferUsage::StaticDraw);
        cubeIndices.setData(MeshTools::compressIndicesAs<UnsignedShort>(data.indices()), GL::BufferUsage::StaticDraw);
        _cubeMesh.setCount(data.indices().size())
               .setPrimitive(data.primitive())
               .addVertexBuffer(cubeVertices, 0, Figure::PhongIdShader::Position{}, Figure::PhongIdShader::Normal{})
               .setIndexBuffer(cubeIndices, 0, MeshIndexType::UnsignedShort);
    }


    prepare3DFont();

    //prepareDrawables();
}

void GraphicsCtx::prepareGLBuffers(const Range2Di& viewport) {
    GL::defaultFramebuffer.setViewport(viewport);

    // Prepare the object select buffer;
    _objectId.setStorage(GL::RenderbufferFormat::R32UI, viewport.size());

    _objselect_framebuffer = GL::Framebuffer{viewport};

    _objselect_framebuffer.attachRenderbuffer(GL::Framebuffer::ColorAttachment{0}, _objectId)
                .mapForDraw({{Figure::PhongIdShader::ObjectIdOutput, GL::Framebuffer::ColorAttachment{0}}});

    CORRADE_INTERNAL_ASSERT(_objselect_framebuffer.checkStatus(GL::FramebufferTarget::Draw) == GL::Framebuffer::Status::Complete);
}

void GraphicsCtx::destroyGLBuffers() {
   _objselect_framebuffer.detach(GL::Framebuffer::ColorAttachment{0});
}

void GraphicsCtx::prepare3DFont() {
    /* Load FreeTypeFont plugin */
    _font = _manager.loadAndInstantiate("FreeTypeFont");
    if(!_font) std::exit(1);

    /* Open the font and fill glyph cache */
    Utility::Resource rs("monopticon");
    std::string fname = "src/assets/DejaVuSans.ttf";

    if(!_font->openData(rs.getRaw(fname), 110.0f)) {
       Fatal{} << "Cannot open font file";
    }

    _font->fillGlyphCache(_glyphCache, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:/-+,.! \n");

     auto inner = 0x00ff00_rgbf;
     auto outline = 0x00ff00_rgbf;
     _text_shader.setColor(inner)
           .setOutlineColor(outline)
           .setOutlineRange(0.45f, 0.445f);
}


Monopticon::Device::Stats* GraphicsCtx::createSphere(SceneCtx *sCtx, const std::string mac) {
    Object3D* o = new Object3D{&_scene};

    int num_objs = sCtx->_device_map.size();
    int vlan = num_objs%3;
    Vector2 v = sCtx->nextVlanPos(vlan);
    Vector3 w = Vector3{v.x(), 0.0f, v.y()};

    o->transform(Matrix4::scaling(Vector3{0.25f}));
    o->translate(w);
    o->rotateY(120.0_degf*static_cast<float>(vlan));
    //o->rotateY(120.0_degf*(float)vlan);

    UnsignedByte id = sCtx->newObjectId();

    Color3 c = 0xa5c9ea_rgbf;
    Figure::DeviceDrawable *dev = new Figure::DeviceDrawable{
        id,
        *o,
        _phong_id_shader,
        c,
        _sphere,
        Matrix4{},
        _selectable_drawables};


    auto tmp = o->transformationMatrix().translation();

    Device::Stats* d_s = new Device::Stats{mac, tmp, dev};
    dev->_deviceStats = d_s;

    sCtx->_selectable_objects.push_back(d_s);
    sCtx->_device_map.insert(std::make_pair(mac, d_s));

    return d_s;
}

void GraphicsCtx::addDirectLabels(Device::Stats *d_s) {
    auto scaling = Matrix4::scaling(Vector3{0.10f});
    auto t = d_s->circPoint;

    int num_ips = d_s->_emitted_src_ips.size();
    if (num_ips > 0 && num_ips < 3) {
        if (d_s->_ip_label != nullptr) {
            delete d_s->_ip_label;
        }

        Object3D *obj = new Object3D{&_scene};
        obj->transform(scaling);

        float offset = num_ips*0.2f;
        obj->translate(t+Vector3(0.0f, 0.5f+offset, 0.0f));

        auto c = 0xeeeeee_rgbf;
        d_s->_ip_label = new Figure::TextDrawable(d_s->makeIpLabel(), c, _font, &_glyphCache, _text_shader, *obj, _text_drawables);
    }

    if (d_s->_mac_label == nullptr) {
        Object3D *obj = new Object3D{&_scene};
        obj->transform(scaling);
        obj->translate(t+Vector3(0.0f, -0.7f, 0.0f));

        auto c = 0xaaaaaa_rgbf;
        d_s->_mac_label = new Figure::TextDrawable(d_s->mac_addr, c, _font, &_glyphCache, _text_shader, *obj, _text_drawables);
    }
}


void GraphicsCtx::addL2ConnectL3(Vector3 a, Vector3 b) {
    Object3D *j = new Object3D{&_scene};
    auto *line = new Figure::RingDrawable(*j, 0x999999_rgbf, _drawables);

    auto c = (b-a).normalized();

    line->setMesh(Primitives::line3D(a,a+c));
}

void GraphicsCtx::createLines(Vector3 a, Vector3 b, Util::L3Type t, int count) {
    int ceiling = std::min(count, 4);
    for (int i = 0; i < ceiling; i++) {
        createLine(a, b, t);
    }
}

void GraphicsCtx::createLine(Vector3 a, Vector3 b, Util::L3Type t) {
    Object3D* line = new Object3D{&_scene};

    Color3 c = Util::typeColor(t);
    Color4 c4;
    /*
    if (_selectedDevice != nullptr) {
        c4 = Color4(c, 0.5);

        auto v = _selectedDevice->circPoint;
        if (a == v || b == v) {
            c4 = Color4(c, 1.00);
        }
    } else {
        c4 = Color4(c, 1.0);
    }
    */
    c4 = Color4(c, 1.0);
    // TODO delete line above

    auto *pl = new Figure::PacketLineDrawable{*line, _line_shader, a, b, _drawables, c4};
    // TODO insert move INSERT into sCtx SceneContext
    //_packet_line_queue.insert(pl);
}

void GraphicsCtx::draw3DElements() {
    // Ensure the custom framebuffer is clear for the draw
    _objselect_framebuffer
        .clear(GL::FramebufferClear::Color)
        .bind();

    // Draw selectable objects to custom framebuffer
    _camera->draw(_selectable_drawables);

    /* Bind the main buffer back */
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth)
        .bind();

    GL::Renderer::setClearColor(_clearColor);

    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

    _camera->draw(_permanent_drawables);
    _camera->draw(_selectable_drawables);
    _camera->draw(_drawables);

    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);

}



SceneCtx::SceneCtx() {



}


UnsignedByte SceneCtx::newObjectId() {
    return static_cast<UnsignedByte>(_selectable_objects.size());
}


Vector2 SceneCtx::nextVlanPos(const int vlan) {
    int row_size = 4 + vlan - vlan;

    int num_objs_in_vlan = _device_map.size()/3; // NOTE replace with vlan

    int vlan_x = num_objs_in_vlan / row_size;
    int vlan_y = num_objs_in_vlan % row_size;

    return 4*Vector2(vlan_x+1, vlan_y+1);
}

