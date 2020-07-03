#include "evenbettercap.h"

namespace Monopticon { namespace Context {

Graphic::Graphic() : _glyphCache(Vector2i(2048), Vector2i(512), 22)
{
    MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GLES300);

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
    _phong_shader = Shaders::Phong{Shaders::Phong::Flag::ObjectId};
    _pool_shader = Figure::PoolShader{};
    _link_shader = Figure::WorldLinkShader{};

    _bbitem_shader = Shaders::Flat3D{};
    _bbitem_shader.setColor(0x00ff00_rgbf);

    {
        Trade::MeshData data = Primitives::uvSphereSolid(8.0f, 30.0f);
        _sphere = MeshTools::compile(data);
    }
    {
        Trade::MeshData data = Primitives::cubeSolid();
        _cubeMesh = MeshTools::compile(data);
    }

    prepare3DFont();
}

void Graphic::prepareGLBuffers(const Range2Di &viewport)
{
    GL::defaultFramebuffer.setViewport(viewport);

    // Prepare the object select buffer;
    _objectId.setStorage(GL::RenderbufferFormat::R32UI, viewport.size());

    _objselect_framebuffer = GL::Framebuffer{viewport};
    _objselect_framebuffer.attachRenderbuffer(GL::Framebuffer::ColorAttachment{1}, _objectId)
        .mapForDraw({{Shaders::Phong::ObjectIdOutput, GL::Framebuffer::ColorAttachment{1}}});


    CORRADE_INTERNAL_ASSERT(_objselect_framebuffer.checkStatus(GL::FramebufferTarget::Draw) == GL::Framebuffer::Status::Complete);
}

void Graphic::destroyGLBuffers()
{
    _objselect_framebuffer.detach(GL::Framebuffer::ColorAttachment{1});
}

void Graphic::prepare3DFont()
{
    /* Load FreeTypeFont plugin */
    _font = _manager.loadAndInstantiate("FreeTypeFont");
    if (!_font)
        std::exit(1);

    /* Open the font and fill glyph cache */
    Utility::Resource rs("monopticon");
    std::string fname = "src/assets/DejaVuSans.ttf";

    if (!_font->openData(rs.getRaw(fname), 110.0f))
    {
        Fatal{} << "Cannot open font file";
    }

    _font->fillGlyphCache(_glyphCache, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:/-+,.! \n");

    auto inner = 0x00ff00_rgbf;
    auto outline = 0x00ff00_rgbf;
    _text_shader.setColor(inner)
        .setOutlineColor(outline)
        .setOutlineRange(0.45f, 0.445f);
}

Device::Stats *Graphic::createSphere(Store *sCtx, const std::string mac)
{

    int num_objs = sCtx->_device_map.size();
    int vlan = num_objs % 3;
    Vector2 v = sCtx->nextVlanPos(vlan);
    Vector3 w = Vector3{v.x(), 0.0f, v.y()};

    return createSphere(sCtx, mac, &_scene, w);
}

Device::Stats *Graphic::createSphere(Store *sCtx, const std::string mac, Object3D *parent, Vector3 relPos)
{
    Object3D *o = new Object3D{parent};
    Object3D *sphere_obj = new Object3D{o};
    o->translate(relPos);
    sphere_obj->transform(Matrix4::scaling(Vector3{0.25f}));

    //o->rotateY(120.0_degf * static_cast<float>(vlan));

    UnsignedByte id = sCtx->newObjectId();

    Color3 c = 0xa5c9ea_rgbf;
    Figure::DeviceDrawable *dev = new Figure::DeviceDrawable{
        id,
        *sphere_obj,
        _phong_shader,
        c,
        _sphere,
        Matrix4{},
        _selectable_drawables};


    Device::Stats *d_s = new Device::Stats{mac, o, dev};
    dev->_deviceStats = d_s;

    sCtx->_selectable_objects.push_back(d_s);
    sCtx->_device_map.insert(std::make_pair(mac, d_s));

    return d_s;
}

Monopticon::Level3::Address *Graphic::createIPv4Address(Context::Store *sCtx, const std::string ipv4_addr, Vector3 pos)
{
    Vector3 offset{0.0f, 1.0f, 0.0f};
    auto t = pos + offset;

    Object3D *g = new Object3D{&_scene};
    Object3D *o = new Object3D{g};

    auto s = Matrix4::scaling(Vector3{0.25f});
    o->transform(s);
    o->translate(t);

    Color3 c = 0xffffff_rgbf;

    UnsignedByte id = sCtx->newObjectId();

    Level3::Address *address_obj = new Level3::Address{
        id,
        *o,
        _phong_shader,
        c,
        _cubeMesh,
        _selectable_drawables};

    sCtx->_selectable_objects.push_back(address_obj);

    Object3D *obj = new Object3D{&_scene};
    auto scaling = Matrix4::scaling(Vector3{0.10f});
    obj->transform(scaling);

    auto p = t + Vector3(0.0f, 0.5f, 0.0f);
    obj->translate(p);

    c = 0xeeeeee_rgbf;
    new Figure::TextDrawable(ipv4_addr, c, _font, &_glyphCache, _text_shader, *obj, _text_drawables);

    return address_obj;
}

Monopticon::Device::PrefixStats *Graphic::createBroadcastPool(const std::string mac_prefix, Vector3 pos)
{
    auto *ring = Util::createLayoutRing(_scene, _permanent_drawables, 1.0f, pos);

    // Add a label to the bcast ring
    auto scaling = Matrix4::scaling(Vector3{0.10f});
    Object3D *obj = new Object3D{&_scene};
    obj->transform(scaling);
    obj->translate(pos);
    auto c = 0xaaaaaa_rgbf;
    new Figure::TextDrawable(mac_prefix, c, _font, &_glyphCache, _text_shader, *obj, _text_drawables);

    Device::PrefixStats *dp_s = new Device::PrefixStats{mac_prefix, pos, ring};

    return dp_s;
}

void Graphic::createPoolHits(Context::Store *sCtx, Device::Stats *tran_d_s, Device::PrefixStats *dp_s, epoch::L2Summary sum)
{
    using namespace Monopticon::Util;

    if (sum.ipv4() > 0)
    {
        createPoolHit(dp_s, typeColor(L3Type::IPV4));
        createLine(sCtx, tran_d_s->circPoint, dp_s->_position, L3Type::IPV4);
    }
    if (sum.ipv6() > 0)
    {
        createPoolHit(dp_s, typeColor(L3Type::IPV6));
        createLine(sCtx, tran_d_s->circPoint, dp_s->_position, L3Type::IPV6);
    }
    if (sum.arp() > 0)
    {
        createPoolHit(dp_s, typeColor(L3Type::ARP));
        createLine(sCtx, tran_d_s->circPoint, dp_s->_position, L3Type::ARP);
    }
    if (sum.unknown() > 0)
    {
        createPoolHit(dp_s, typeColor(L3Type::UNKNOWN));
        createLine(sCtx, tran_d_s->circPoint, dp_s->_position, L3Type::UNKNOWN);
    }

    tran_d_s->num_pkts_sent += Util::SumTotal(sum);
}

void Graphic::createPoolHit(Device::PrefixStats *dp_s, Color3 c)
{
    // No-op if the contact pool is already drawing lots of rings
    if (dp_s->contacts.size() > 9)
    {
        return;
    }
    auto pos = dp_s->_position;

    auto scaling = Matrix4::scaling(Vector3{1.0f});
    Object3D *o = new Object3D{&_scene};
    o->transform(scaling);
    o->rotateX(90.0_degf);
    o->translate(pos);

    Object3D *u = new Object3D{&_scene};
    u->transform(scaling);
    u->rotateX(270.0_degf);
    u->translate(pos);

    auto top = new Figure::MulticastDrawable(*u, c, pos, _pool_shader, _drawables, _poolCircle);
    auto bot = new Figure::MulticastDrawable(*o, c, pos, _pool_shader, _drawables, _poolCircle);
    auto pair = std::make_pair(top, bot);

    dp_s->contacts.push_back(pair);
}


Layout::Router* Graphic::createRouter(Store *sCtx, Vector3 pos, std::string label, std::vector<Layout::RInput*> ifaces)
{
    Object3D *root = new Object3D{&_scene};
    root->translate(pos);

    Layout::Router* router = new Layout::Router(0, ifaces.size(), root);
    auto grid = Util::createLayoutRing(*root, _permanent_drawables, 2.0f, Vector3{});

    // TODO store min and max x and y and generate connecting rectangle of that size.
    for (int i = 0; i < ifaces.size(); i++) {
        Layout::RInput* params = ifaces.at(i);

        Vector3 relPos = Vector3(params->pos.x(), 0.0f, params->pos.y());

        // Create Device Stats
        Device::Stats* d_s = createSphere(sCtx, params->mac, root, relPos);
        addDirectLabels(d_s);

        // TODO project relPos from 0,0 with a ray to choose a good spot to place the vlan bcast pool

        // TODO uses `i` instead of vlan tag
        router->plugIface(d_s, (uint32_t)i);
    }


    return router;
}


void Graphic::addDirectLabels(Device::Stats *d_s)
{
    auto scaling = Matrix4::scaling(Vector3{0.10f});
    auto t = d_s->circPoint;

    int num_ips = d_s->_emitted_src_ips.size();
    if (num_ips > 0 && num_ips < 3)
    {
        if (d_s->_ip_label != nullptr)
        {
            delete d_s->_ip_label;
        }

        Object3D *obj = new Object3D{&_scene};
        obj->transform(scaling);

        float offset = num_ips * 0.2f;
        obj->translate(t + Vector3(0.0f, 0.5f + offset, 0.0f));

        auto c = 0xeeeeee_rgbf;
        d_s->_ip_label = new Figure::TextDrawable(d_s->makeIpLabel(), c, _font, &_glyphCache, _text_shader, *obj, _text_drawables);
    }

    if (d_s->_mac_label == nullptr)
    {
        Object3D *obj = new Object3D{d_s->_root_obj};
        obj->transform(scaling);
        obj->translate(Vector3(0.0f, -0.7f, 0.0f));

        Color3 c = 0xaaaaaa_rgbf;
        d_s->_mac_label = new Figure::TextDrawable(d_s->mac_addr, c, _font, &_glyphCache, _text_shader, *obj, _text_drawables);
    }
}

void Graphic::addL2ConnectL3(Vector3 a, Vector3 b)
{
    Object3D *j = new Object3D{&_scene};
    auto *line = new Figure::RingDrawable(*j, 0x999999_rgbf, _drawables);

    auto c = (b - a).normalized();

    line->setMesh(Primitives::line3D(a, a + c));
}

void Graphic::createLines(Context::Store *sCtx, Vector3 a, Vector3 b, Util::L3Type t, int count)
{
    int ceiling = std::min(count, 4);
    for (int i = 0; i < ceiling; i++)
    {
        createLine(sCtx, a, b, t);
    }
}

void Graphic::createLine(Context::Store *sCtx, Vector3 a, Vector3 b, Util::L3Type t)
{
    Object3D *line = new Object3D{&_scene};

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

void Graphic::draw3DElements()
{
    // Ensure the custom framebuffer is clear for the draw
    _objselect_framebuffer
        .clearColor(1, Vector4ui{})
        .bind();

    // Draw selectable objects to custom framebuffer
    _camera->draw(_selectable_drawables);

    /* Bind the main buffer back */
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth)
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

Store::Store()
{
}

UnsignedByte Store::newObjectId()
{
    return static_cast<UnsignedByte>(_selectable_objects.size());
}

Vector2 Store::nextVlanPos(const int vlan)
{
    int row_size = 4 + vlan - vlan;

    int num_objs_in_vlan = _device_map.size() / 3; // NOTE replace with vlan

    int vlan_x = num_objs_in_vlan / row_size;
    int vlan_y = num_objs_in_vlan % row_size;

    return 4 * Vector2(vlan_x + 1, vlan_y + 1);
}


void Store::frameUpdate() {
    // Remove packet_lines that have expired from the queue
    std::set<Figure::PacketLineDrawable *>::iterator it;
    for (it = _packet_line_queue.begin(); it != _packet_line_queue.end(); ) {
        // Note this is an O(N) operation
        Figure::PacketLineDrawable *pl = *it;
        if (pl->_expired) {
            it = _packet_line_queue.erase(it);
            delete pl;
        } else {
            ++it;
        }
    }

    // Remove mcast drawables that have expired
    for (auto it2 = _dst_prefix_group_map.begin(); it2 != _dst_prefix_group_map.end(); it2++) {
        Device::PrefixStats *dp_s = (*it2).second;
        std::vector<std::pair<Figure::MulticastDrawable*, Figure::MulticastDrawable*>> c = dp_s->contacts;
        for (auto it3 = c.begin(); it3 != c.end(); ) {
            auto pair = *it3;
            if ((pair.first)->expired) {
                it3 = c.erase(it3);
                delete pair.first;
                delete pair.second;
            } else {
                ++it3;
            }
        }
        dp_s->contacts = c;
    }
}

} // Context
} // Monopticon