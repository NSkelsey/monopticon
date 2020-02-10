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


Monopticon::Level3::Address* GraphicsCtx::createIPv4Address(Context::SceneCtx *sCtx, const std::string ipv4_addr, Vector3 pos) {
    Vector3 offset{0.0f, 1.0f, 0.0f};
    auto t = pos+offset;

    //std::cout << ipv4_addr << std::endl;
    //Utility::Debug{} << t;

    Object3D* g = new Object3D{&_scene};
    Object3D* o = new Object3D{g};

    auto s = Matrix4::scaling(Vector3{0.25f});
    o->transform(s);
    o->translate(t);

    Color3 c = 0xffffff_rgbf;

    UnsignedByte id = sCtx->newObjectId();

    Level3::Address *address_obj = new Level3::Address {
        id,
        *o,
        _phong_id_shader,
        c,
        _cubeMesh,
        _selectable_drawables
    };

    sCtx->_selectable_objects.push_back(address_obj);

    Object3D *obj = new Object3D{&_scene};
    auto scaling = Matrix4::scaling(Vector3{0.10f});
    obj->transform(scaling);

    auto p = t+Vector3(0.0f, 0.5f, 0.0f);
    obj->translate(p);

    c = 0xeeeeee_rgbf;
    new Figure::TextDrawable(ipv4_addr, c, _font, &_glyphCache, _text_shader, *obj, _text_drawables);

    return address_obj;
}


Monopticon::Device::PrefixStats* GraphicsCtx::createBroadcastPool(const std::string mac_prefix, Vector3 pos) {
    auto *ring = Util::createLayoutRing(_scene, _permanent_drawables, 1.0f, pos);

    // Add a label to the bcast ring
    auto scaling = Matrix4::scaling(Vector3{0.10f});
    Object3D *obj = new Object3D{&_scene};
    obj->transform(scaling);
    obj->translate(pos);
    auto c = 0xaaaaaa_rgbf;
    new Figure::TextDrawable(mac_prefix, c, _font, &_glyphCache, _text_shader, *obj, _text_drawables);

    Device::PrefixStats* dp_s = new Device::PrefixStats{mac_prefix, pos, ring};

    return dp_s;
}


void GraphicsCtx::createPoolHits(Context::SceneCtx *sCtx, Device::Stats* tran_d_s, Device::PrefixStats *dp_s, Util::L2Summary sum) {
    using namespace Monopticon::Util;

    if (sum.ipv4_cnt > 0) {
        createPoolHit(sCtx, dp_s, typeColor(L3Type::IPV4));
        createLine(sCtx, tran_d_s->circPoint, dp_s->_position, L3Type::IPV4);
    }
    if (sum.ipv6_cnt > 0) {
        createPoolHit(sCtx, dp_s, typeColor(L3Type::IPV6));
        createLine(sCtx, tran_d_s->circPoint, dp_s->_position, L3Type::IPV6);
    }
    if (sum.arp_cnt > 0) {
        createPoolHit(sCtx, dp_s, typeColor(L3Type::ARP));
        createLine(sCtx, tran_d_s->circPoint, dp_s->_position, L3Type::ARP);
    }
    if (sum.unknown_cnt > 0) {
        createPoolHit(sCtx, dp_s, typeColor(L3Type::UNKNOWN));
        createLine(sCtx, tran_d_s->circPoint, dp_s->_position, L3Type::UNKNOWN);
    }

    tran_d_s->num_pkts_sent += Util::SumTotal(sum);
}


void GraphicsCtx::createPoolHit(Context::SceneCtx *sCtx, Device::PrefixStats *dp_s, Color3 c) {
    // No-op if the contact pool is already drawing lots of rings
    if (dp_s->contacts.size() > 9) {
        return;
    }
    auto pos = dp_s->_position;

    auto scaling = Matrix4::scaling(Vector3{1.0f});
    Object3D* o = new Object3D{&_scene};
    o->transform(scaling);
    o->rotateX(90.0_degf);
    o->translate(pos);

    Object3D* u = new Object3D{&_scene};
    u->transform(scaling);
    u->rotateX(270.0_degf);
    u->translate(pos);

    auto top = new Figure::MulticastDrawable(*u, c, pos, _pool_shader, _drawables, _poolCircle);
    auto bot = new Figure::MulticastDrawable(*o, c, pos, _pool_shader, _drawables, _poolCircle);
    auto pair = std::make_pair(top, bot);

    dp_s->contacts.push_back(pair);
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

void GraphicsCtx::createLines(Context::SceneCtx *sCtx, Vector3 a, Vector3 b, Util::L3Type t, int count) {
    int ceiling = std::min(count, 4);
    for (int i = 0; i < ceiling; i++) {
        createLine(sCtx, a, b, t);
    }
}

void GraphicsCtx::createLine(Context::SceneCtx *sCtx, Vector3 a, Vector3 b, Util::L3Type t) {
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
    sCtx->_packet_line_queue.insert(pl);
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

