#include "evenbettercap.h"

namespace Monopticon {

const int MSAA_CNT = 8; // Number of subpixel samples for MultiSampling Anti-Aliasing

// Layout constants
const int num_rings = 8;
const int elems_per_ring[8]{1, 4, 8, 16, 32, 64, 256, 10000};
const float ring_radii[8]{0.0f, 4.0f, 8.0f, 12.0f, 16.0f, 24.0f, 32.0f, 64.0f};

const Vector3 offset{0.0f, 1.0f, 0.0f};

using namespace Magnum;
using namespace Math::Literals;


class Application: public Platform::Application {
    public:
        explicit Application(const Arguments& arguments);

        void prepare3DFont();
        void prepareDrawables();
        void prepareGLBuffers(const Range2Di& viewport);

        void destroyGLBuffers();

        void drawEvent() override;
        int processNetworkEvents();
        void drawTextElements();
        void draw3DElements();
        void drawIMGuiElements(int event_cnt);

        void viewportEvent(ViewportEvent& event) override;

        void keyPressEvent(KeyEvent& event) override;
        void keyReleaseEvent(KeyEvent& event) override;

        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;
        void mouseScrollEvent(MouseScrollEvent& event) override;
        void textInputEvent(TextInputEvent& event) override;

        Device::Stats* createSphere(const std::string);
  
        Level3::Address* createFakeIPv4Address(const std::string ipv4_addr, Vector3 pos);

        void createLines(Vector3, Vector3, Util::L3Type, int num);
        void createLine(Vector3, Vector3, Util::L3Type);

        void addDirectLabels(Device::Stats *d_s);
        void addL2ConnectL3(Vector3 a, Vector3 b);

        UnsignedByte newObjectId();
        void deselectObject();
        void objectClicked(Device::Selectable *selection);
        void selectableMenuActions(Device::Selectable *selection);
        void watchSelectedDevice();

        Vector2 nextVlanPos(const int vlan);


    private:
        // UI fields
        ImGuiIntegration::Context _imgui{NoCreate};

        // Graphic fields
        GL::Mesh _sphere{}, _poolCircle{NoCreate}, _cubeMesh{};
        Color4 _clearColor = 0x002b36_rgbf;
        Color3 _pickColor = 0xffffff_rgbf;

        Shaders::Phong _phong_shader;
        Figure::PhongIdShader _phong_id_shader;
        Figure::ParaLineShader _line_shader;
        Figure::PoolShader _pool_shader;
        Figure::WorldLinkShader _link_shader;
        Shaders::Flat3D _bbitem_shader;

        Scene3D _scene;
        SceneGraph::Camera3D* _camera;
        SceneGraph::DrawableGroup3D _drawables;
        SceneGraph::DrawableGroup3D _permanent_drawables;
        SceneGraph::DrawableGroup3D _selectable_drawables;
        SceneGraph::DrawableGroup3D _billboard_drawables;
        SceneGraph::DrawableGroup3D _text_drawables;
        Timeline _timeline;

        Object3D *_cameraRig, *_cameraObject;

        GL::Framebuffer _objselect_framebuffer{NoCreate};
        Vector2i _previousMousePosition, _mousePressPosition;
        GL::Renderbuffer _color, _objectId, _depth;


        Movement::TranslateController *_translateController;

        // Font graphics fields
        PluginManager::Manager<Text::AbstractFont> _manager;
        Containers::Pointer<Text::AbstractFont> _font;

        Text::DistanceFieldGlyphCache _glyphCache;
        Shaders::DistanceFieldVector3D _text_shader;

        // Scene objects
        std::vector<Device::Selectable*> _selectable_objects{};
        std::set<Figure::PacketLineDrawable*> _packet_line_queue{};

        std::map<std::string, Device::Stats*> _device_map{};
        std::map<std::string, Device::PrefixStats*> _dst_prefix_group_map{};
        std::map<std::string, Device::PrefixStats*> _prefix_group_map{};

        std::vector<Device::WindowMgr*> _inspected_device_window_list{};

        Device::Selectable* _selectedObject{nullptr};
        Device::Stats* _listeningDevice{nullptr};
        Device::Stats* _activeGateway{nullptr};

        // Network interface state tracking
        std::vector<std::string> _iface_list;
        std::string _chosen_iface;

        std::string _zeek_pid;

        // Custom ImGui interface components
        Device::ChartMgr ifaceChartMgr{240, 3.0f};
        Device::ChartMgr ifaceLongChartMgr{300, 3.0f};

        int ring_level{0};
        int pos_in_ring{0};

        int run_sum;
        int frame_cnt;
        std::chrono::duration<int64_t, std::nano> curr_pkt_lag;

        int tot_pkt_drop;
        int tot_epoch_drop;

        bool _orbit_toggle{false};
        bool _openPopup{false};

        int inv_sample_rate{1};
};


Application::Application(const Arguments& arguments):
        Platform::Application{arguments, Configuration{}
            .setTitle("Monopticon")
            .setWindowFlags(Configuration::WindowFlag::Borderless|Configuration::WindowFlag::Resizable)
            .setSize(Vector2i{1400,1000}),
            GLConfiguration{}.setSampleCount(MSAA_CNT)},
        _glyphCache(Vector2i(2048), Vector2i(512), 22)
{
    MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL330);

    // Setup the SDL window icon
    Utility::Resource rs("monopticon");
    std::string s = rs.get("icon.bmp");
    SDL_RWops *rw = SDL_RWFromConstMem(s.c_str(), s.size());

    SDL_Surface* sdl_surf = SDL_LoadBMP_RW(rw, 1);
    if (sdl_surf == nullptr) {
        std::cerr << "Could not set window icon. SDL startup failed.";
        std::exit(1);
    }
    SDL_Window* sdl_window = Magnum::Platform::Sdl2Application::window();
    SDL_SetWindowIcon(sdl_window, sdl_surf);

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

    _poolCircle = MeshTools::compile(Primitives::circle3DWireframe(20));

    _line_shader = Figure::ParaLineShader{};
    _phong_id_shader = Figure::PhongIdShader{};
    _pool_shader = Figure::PoolShader{};
    _link_shader = Figure::WorldLinkShader{};

    _bbitem_shader = Shaders::Flat3D{};
    _bbitem_shader.setColor(0x00ff00_rgbf);

    srand(time(nullptr));

    _imgui = ImGuiIntegration::Context(Vector2{windowSize()}/dpiScaling(),
        windowSize(), framebufferSize());

    run_sum = 0;
    frame_cnt = 0;
    tot_pkt_drop = 0;
    tot_epoch_drop = 0;
    curr_pkt_lag = broker::timespan(0);

    setSwapInterval(1);
    setMinimalLoopPeriod(16);
    _timeline.start();

    _iface_list = Util::get_iface_list();
    _zeek_pid = "#nop";

    prepare3DFont();

    prepareDrawables();
}


void Application::prepareGLBuffers(const Range2Di& viewport) {
    GL::defaultFramebuffer.setViewport(viewport);

    // Prepare the object select buffer;
    _objectId.setStorage(GL::RenderbufferFormat::R32UI, viewport.size());

    _objselect_framebuffer = GL::Framebuffer{viewport};

    _objselect_framebuffer.attachRenderbuffer(GL::Framebuffer::ColorAttachment{0}, _objectId)
                .mapForDraw({{Figure::PhongIdShader::ObjectIdOutput, GL::Framebuffer::ColorAttachment{0}}});

    CORRADE_INTERNAL_ASSERT(_objselect_framebuffer.checkStatus(GL::FramebufferTarget::Draw) == GL::Framebuffer::Status::Complete);
}


void Application::prepareDrawables() {
    //Device::PrefixStats *ff_bcast = createBroadcastPool("ff", Vector3{1.0f, -4.0f, 1.0f});
    //Device::PrefixStats *three_bcast = createBroadcastPool("33", Vector3{1.0f, -4.0f, -1.0f});
    //Device::PrefixStats *one_bcast = createBroadcastPool("01", Vector3{-1.0f, -4.0f, 1.0f});
    //Device::PrefixStats *odd_bcast = createBroadcastPool("odd", Vector3{-1.0f, -4.0f, -1.0f});

    //_dst_prefix_group_map.insert(std::make_pair("ff", ff_bcast));
    //_dst_prefix_group_map.insert(std::make_pair("33", three_bcast));
    //_dst_prefix_group_map.insert(std::make_pair("01", one_bcast));
    //_dst_prefix_group_map.insert(std::make_pair("odd", odd_bcast));
    _translateController = new Movement::TranslateController{&_scene, &_drawables};
    
    auto trans = Vector3{0.0f, 0.0f, 0.0f};

    createFakeIPv4Address("111.111.111.111", trans);

    auto trans2 = Vector3{3.0f, 0.0f, 3.0f};

    createFakeIPv4Address("8.8.8.8", trans2);
/*
    auto trans = Vector3{1.0f, -4.0f, 1.0f};

    Object3D *obj = new Object3D{_translateController};
    Matrix4 scaling = Matrix4::scaling(Vector3{1.0});
    obj->transform(scaling);
    obj->rotateX(90.0_degf);
    obj->translate(trans);
    auto ring = new Figure::RingDrawable{*obj, 0xcccccc_rgbf, _drawables};

    // Add a label to the bcast ring
    auto scaling2 = Matrix4::scaling(Vector3{0.10f});
    Object3D *obj2 = new Object3D{&_scene};
    obj2->transform(scaling2);
    obj2->translate(trans);
    auto c = 0xaaaaaa_rgbf;
    new Figure::TextDrawable("BB:BB:BB", c, _font, &_glyphCache, _text_shader, *obj2, _text_drawables);

    //Device::PrefixStats* dp_s = new Device::PrefixStats{"BB:BB:BB", trans, ring};

    */
}


void Application::destroyGLBuffers() {
   _objselect_framebuffer.detach(GL::Framebuffer::ColorAttachment{0});
}


void Application::prepare3DFont() {
    /* Load FreeTypeFont plugin */
    _font = _manager.loadAndInstantiate("FreeTypeFont");
    if(!_font) std::exit(1);

    /* Open the font and fill glyph cache */
    Utility::Resource rs("monopticon");

    std::string fname = "src/assets/DejaVuSans.ttf";
    Containers::ArrayView<const char> a = rs.getRaw(fname);

    std::vector<std::pair<std::string, Containers::ArrayView<const char>>> f_pair = {std::make_pair(fname, a)};

    if(!_font->openData(f_pair, 110.0f)) {
       Error() << "Cannot open font file";
       std::exit(1);
    }

    _font->fillGlyphCache(_glyphCache, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:/-+,.! \n");

     auto inner = 0x00ff00_rgbf;
     auto outline = 0x00ff00_rgbf;
     _text_shader.setColor(inner)
           .setOutlineColor(outline)
           .setOutlineRange(0.45f, 0.445f);
}


void Application::drawTextElements() {
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha, GL::Renderer::BlendFunction::OneMinusSourceAlpha);
    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add, GL::Renderer::BlendEquation::Add);

    _text_shader.bindVectorTexture(_glyphCache.texture());

    _camera->draw(_text_drawables);

    GL::Renderer::disable(GL::Renderer::Feature::Blending);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::Zero);
    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add, GL::Renderer::BlendEquation::Add);

    _camera->draw(_billboard_drawables);
}


void Application::draw3DElements() {
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


void Application::drawIMGuiElements(int event_cnt) {
    _imgui.newFrame();

    if (_openPopup) {
        ImGui::OpenPopup("selectablePopup");
        _openPopup = false;
    }

    if (ImGui::BeginPopup("selectablePopup")) {
        selectableMenuActions(_selectedObject);
        ImGui::EndPopup();
    }

    ImGui::SetNextWindowSize(ImVec2(315, 245), ImGuiCond_Always);
    auto flags = ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoScrollbar;
    ImGui::Begin("Tap Status", nullptr, flags);

    ImGui::Text("App average %.3f ms/frame (%.1f FPS)",
            1000.0/Magnum::Double(ImGui::GetIO().Framerate), Magnum::Double(ImGui::GetIO().Framerate));

    auto s = "Sample rate %.3f SPS event cnt %d";
    if (inv_sample_rate > 1.0) {
        ImGui::TextColored(ImVec4(1,0,0,1), s, 1.0/inv_sample_rate, event_cnt);
    } else {
        ImGui::Text(s, 1.0/inv_sample_rate, event_cnt);
    }

    auto r = "Pkt Lag %.1f ms; Pkt drop: %d; Epoch drop: %d";
    double t = curr_pkt_lag.count()/1000000.0;
    if (curr_pkt_lag > std::chrono::milliseconds(5)) {
        ImGui::TextColored(ImVec4(1,0,0,1), r, t, tot_pkt_drop, tot_epoch_drop);
    } else {
        ImGui::Text(r, t, tot_pkt_drop, tot_epoch_drop);
    }

    ImGui::Separator();
    ifaceChartMgr.draw();
    ImGui::Separator();
    ifaceLongChartMgr.draw();
    ImGui::Separator();

    ImGui::End();

    ImGui::SetNextWindowSize(ImVec2(315, 215), ImGuiCond_Once);
    ImGui::Begin("Heads Up Display", nullptr, flags);

    if (ImGui::Button("Watch", ImVec2(80,20))) {
        watchSelectedDevice();
    }

    ImGui::SameLine(100.0f);
    if (!_orbit_toggle) {
        if (ImGui::Button("Start Orbit", ImVec2(90,20))) {
               // Rotate the camera on an orbit
               _orbit_toggle = true;
        }
    } else {
       if (ImGui::Button("Stop Orbit", ImVec2(80,20))) {
           _orbit_toggle = false;
       }
       _cameraRig->rotateY(0.10_degf);
    }

    ImGui::Text("Observed Addresses");
    ImGui::Separator();
    ImGui::BeginChild("Scrolling");
    int i = 1;
    for (auto it = _device_map.begin(); it != _device_map.end(); it++) {
        Device::Stats *d_s = it->second;

        char b[4] = {};
        sprintf(b, "%d", i);
        i++;
        if (ImGui::InvisibleButton(b, ImVec2(300, 18))) {
            objectClicked(d_s);
        }
        ImGui::SameLine(5.0f);
        d_s->renderText();
    }

    ImGui::EndChild();

    ImGui::End();

    // Render custom windows chosen by user
    for (auto it = _inspected_device_window_list.begin(); it != _inspected_device_window_list.end();) {
        Device::WindowMgr *dwm = *it;
        if (dwm->_win_open) {
            dwm->draw();
            ++it;
        } else {
            delete dwm->_lineDrawable;
            dwm->_stats->_windowMgr = nullptr;
            it = _inspected_device_window_list.erase(it);
        }
    }

    GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::Blending);

    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
        GL::Renderer::BlendEquation::Add);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    _imgui.drawFrame();

    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
}

UnsignedByte Application::newObjectId() {
    return static_cast<UnsignedByte>(_selectable_objects.size());
}


void Application::deselectObject() {
    if (_selectedObject != nullptr) {
        _selectedObject->deleteHighlight();
        _selectedObject = nullptr;
    }
}


void Application::addDirectLabels(Device::Stats *d_s) {
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


void Application::selectableMenuActions(Device::Selectable *selection) {
    int res = selection->rightClickActions();
    if (res == 0) {
        return;
    } else if (res == 1) {
        watchSelectedDevice();
    } else if (res == 2) {
        std::cout << "scan dev" << std::endl;
    } else if (res == 3) {
        std::cout << "change orbit" << std::endl;
    } else {
        std::cerr << "No selectable menu action found " << res << std::endl;
    }
}


void Application::objectClicked(Device::Selectable *selection) {
    deselectObject();
    _selectedObject = selection;

    //Object3D *o = selection->getObj();

    //Level3::Address *a = dynamic_cast<Level3::Address*>(selection);
    selection->addHighlight(_bbitem_shader, _billboard_drawables);
}


Vector2 Application::nextVlanPos(const int vlan) {
    int row_size = 4;

    int num_objs_in_vlan = _device_map.size()/3; // NOTE replace with vlan

    int vlan_x = num_objs_in_vlan / row_size;
    int vlan_y = num_objs_in_vlan % row_size;

    return 4*Vector2(vlan_x+1, vlan_y+1);
}



Device::Stats* Application::createSphere(const std::string mac) {
    Object3D* o = new Object3D{&_scene};

    int num_objs = _device_map.size();
    int vlan = num_objs%3;
    Vector2 v = nextVlanPos(vlan);
    Vector3 w = Vector3{v.x(), 0.0f, v.y()};

    o->transform(Matrix4::scaling(Vector3{0.25f}));
    o->translate(w);
    o->rotateY(120.0_degf*static_cast<float>(vlan));
    //o->rotateY(120.0_degf*(float)vlan);

    UnsignedByte id = newObjectId();

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

    _selectable_objects.push_back(d_s);
    _device_map.insert(std::make_pair(mac, d_s));

    return d_s;
}


Level3::Address* Application::createFakeIPv4Address(const std::string ipv4_addr, Vector3 pos) {
    auto t = pos+offset;

    //std::cout << ipv4_addr << std::endl;
    //Utility::Debug{} << t;

    _translateController = new Movement::TranslateController{&_scene, &_drawables};

    Object3D* g = new Object3D{_translateController};
    Object3D* o = new Object3D{g};

    auto s = Matrix4::scaling(Vector3{0.25f});
    o->transform(s);
    o->translate(t);

    Color3 c = 0xffffff_rgbf;

    UnsignedByte id = newObjectId();

    Level3::Address *address_obj = new Level3::Address {
        id,
        *o,
        _phong_id_shader,
        c,
        _cubeMesh,
        _selectable_drawables
    };

    _selectable_objects.push_back(address_obj);

    Object3D *obj = new Object3D{g};
    auto scaling = Matrix4::scaling(Vector3{0.10f});
    obj->transform(scaling);

    auto p = Vector3(0.0f, 1.5f, 0.0f);
    obj->translate(p);

    c = 0xeeeeee_rgbf;
    new Figure::TextDrawable(ipv4_addr, c, _font, &_glyphCache, _text_shader, *obj, _text_drawables);

    return address_obj;
}





void Application::addL2ConnectL3(Vector3 a, Vector3 b) {
    Object3D *j = new Object3D{&_scene};
    auto *line = new Figure::RingDrawable(*j, 0x999999_rgbf, _drawables);

    auto c = (b-a).normalized();

    line->setMesh(Primitives::line3D(a,a+c));
}




void Application::createLines(Vector3 a, Vector3 b, Util::L3Type t, int count) {
    int ceiling = std::min(count, 4);
    for (int i = 0; i < ceiling; i++) {
        createLine(a, b, t);
    }
}


void Application::createLine(Vector3 a, Vector3 b, Util::L3Type t) {
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
    _packet_line_queue.insert(pl);
}

void Application::watchSelectedDevice() {
    // TODO can move logic into parent.
    auto *selectedDevice = dynamic_cast<Device::Stats*>(_selectedObject);
    if (selectedDevice != nullptr && selectedDevice->_windowMgr != nullptr) {
        std::cerr << "Ref to window already exists" << std::endl;
        return;
    }
    if (selectedDevice != nullptr) {

        Device::WindowMgr *dwm = new Device::WindowMgr(selectedDevice);
        selectedDevice->_windowMgr = dwm;
        _inspected_device_window_list.push_back(dwm);

        auto *obj = new Object3D{&_scene};
        dwm->_lineDrawable = new Figure::WorldScreenLink(*obj, 0xffffff_rgbf, _link_shader, _drawables);
    }
}


void Application::drawEvent() {

    draw3DElements();

    drawTextElements();

    drawIMGuiElements(0);

    frame_cnt ++;
    swapBuffers();
    _timeline.nextFrame();
    redraw();
}


void Application::viewportEvent(ViewportEvent& event) {
    auto size = event.framebufferSize();
    const Range2Di newViewport = {{}, size};

    destroyGLBuffers();
    prepareGLBuffers(newViewport);

    _camera->setViewport(size);

    _imgui.relayout(Vector2{event.windowSize()}/event.dpiScaling(),
        event.windowSize(), size);
}


void Application::keyPressEvent(KeyEvent& event) {
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


void Application::keyReleaseEvent(KeyEvent& event) {
    if(_imgui.handleKeyReleaseEvent(event)) return;
}


void Application::mousePressEvent(MouseEvent& event) {
    if(_imgui.handleMousePressEvent(event)) return;


    if(event.button() == MouseEvent::Button::Left) {
        _previousMousePosition = _mousePressPosition = event.position();
    }

    if (event.button() == MouseEvent::Button::Right) {
        Vector2 screenPoint = Vector2{event.position()} / Vector2{windowSize()};
        Movement::Ray cameraRay = Movement::getCameraToViewportRay(*_camera, screenPoint);

        _translateController->grab(cameraRay);
    }

    event.setAccepted();
}


void Application::mouseReleaseEvent(MouseEvent& event) {
    if(_imgui.handleMouseReleaseEvent(event)) return;

    auto btn = event.button();
    if(!(btn == MouseEvent::Button::Left || btn == MouseEvent::Button::Right)) return;


    if(event.button() == MouseEvent::Button::Right) {
         _translateController->release();
    }

    if(event.button() == MouseEvent::Button::Left) {
    /* Read object ID at given click position (framebuffer has Y up while windowing system Y down) */
    _objselect_framebuffer.mapForRead(GL::Framebuffer::ColorAttachment{0});
    Image2D data = _objselect_framebuffer.read(
        Range2Di::fromSize({event.position().x(), _objselect_framebuffer.viewport().sizeY() - event.position().y() - 1}, {1, 1}),
        {PixelFormat::R32UI});

    deselectObject();
    UnsignedByte id = Containers::arrayCast<UnsignedByte>(data.data())[0];
    unsigned short i = static_cast<unsigned short>(id);
    if(i > 0 && i < _selectable_objects.size()+1) {
        Device::Selectable *selection = _selectable_objects.at(i-1);

        objectClicked(selection);

        if (btn == MouseEvent::Button::Left) {
            _cameraRig->resetTransformation();
            _cameraRig->translate(selection->getTranslation());
        } else if (btn == MouseEvent::Button::Right) {
            _openPopup = true;
        }
    }
    }
    event.setAccepted();
    redraw();
    
}


void Application::mouseMoveEvent(MouseMoveEvent& event) {
    if(_imgui.handleMouseMoveEvent(event)) return;

  if (event.buttons() == MouseMoveEvent::Button::Right) {
    Vector2 screenPoint = Vector2{event.position()} / Vector2{windowSize()};
    Movement::Ray cameraRay = Movement::getCameraToViewportRay(*_camera, screenPoint);

    _translateController->move(cameraRay);
  }

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


void Application::mouseScrollEvent(MouseScrollEvent& event) {
    if(_imgui.handleMouseScrollEvent(event)) return;

    const Vector3 relRotationPoint{0.0, 0.0, -20.0};

    const Float direction = event.offset().y();
    if (!direction) return;

    auto c = _cameraObject->transformationMatrix().translation();
    auto d = relRotationPoint*direction*0.1f;
    auto f = c+d;

    // Calculate distance of new _cameraObject position to local Vec(0.0)
    float distance = sqrt((f*f).sum());

    // Clamp zoom to reasonable levels.
    // Note max distance limit to prevent culling of selectedDevice bounding box.
    // TODO force a final uniform zoom for the closest and farthest position for the camera
    if ((distance < 5.0f && direction > 0.0f) || (distance > 35.0f && direction < 0.0f)) {
        return;
    }
    _cameraObject->translateLocal(d);

    redraw();
}


void Application::textInputEvent(TextInputEvent& event) {
    if(_imgui.handleTextInputEvent(event)) return;
}

}

MAGNUM_APPLICATION_MAIN(Monopticon::Application)
