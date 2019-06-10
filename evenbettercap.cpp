/*
Evenbettercap displays and filters ethernet traffic in 3D
Copyright (C) 2019 Nick Skelsey

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "evenbettercap.h"

namespace Monopticon {

// Zeek broker components
broker::endpoint _ep;
broker::subscriber _subscriber = _ep.make_subscriber({"monopt/l2"});
broker::status_subscriber _status_subscriber = _ep.make_status_subscriber(true);

using namespace Magnum;
using namespace Math::Literals;

class Application: public Platform::Application {
    public:
        explicit Application(const Arguments& arguments);

        void drawEvent() override;

        void viewportEvent(ViewportEvent& event) override;

        void keyPressEvent(KeyEvent& event) override;
        void keyReleaseEvent(KeyEvent& event) override;

        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;
        void mouseScrollEvent(MouseScrollEvent& event) override;
        void textInputEvent(TextInputEvent& event) override;

        void parse_raw_packet(broker::zeek::Event event);
        Device::Stats* createSphere(const std::string);
        void createLine(Vector2, Vector2, Util::L3Type);

        void deselectDevice();
        void deviceClicked(Device::Stats *d_s);
        void highlightDevice(Device::Stats *d_s);

    private:
        // UI fields
        ImGuiIntegration::Context _imgui{NoCreate};

        // Graphic fields
        GL::Mesh _sphere, _circle{NoCreate};
        Color4 _clearColor = 0x002b36_rgbf;
        Color3 _pickColor = 0xffffff_rgbf;

        GL::Buffer _indexBuffer, _vertexBuffer;

        Figure::PhongIdShader _phong_id_shader;
        Figure::ParaLineShader _line_shader;
        Shaders::Flat3D _bbitem_shader;

        Scene3D _scene;
        SceneGraph::Camera3D* _camera;
        SceneGraph::DrawableGroup3D _drawables;
        SceneGraph::DrawableGroup3D _billboard_drawables;
        Timeline _timeline;

        Object3D *_cameraRig, *_cameraObject;

        GL::Framebuffer _framebuffer;
        Vector2i _previousMousePosition, _mousePressPosition;
        GL::Renderbuffer _color, _objectId, _depth;

        GL::Buffer _sphereVertices, _sphereIndices;

        // Scene objects
        std::vector<Figure::DeviceDrawable*> _device_objects{};
        std::set<Figure::PacketLineDrawable*> _packet_line_queue{};

        std::map<std::string, Device::Stats*> _device_map{};

        std::vector<Device::WindowMgr*> _inspected_device_window_list{};

        Device::Stats* _selectedDevice{nullptr};

        // Custom ImGui interface components
        Device::ChartMgr ifaceChartMgr{240, 1.5f};
        Device::ChartMgr ifaceLongChartMgr{300, 3.0f};

        int run_sum;
        int frame_cnt;

        bool _orbit_toggle{false};

        std::vector<std::string> _iface_list;
        std::string _zeek_pid;
};


Application::Application(const Arguments& arguments):
        Platform::Application{arguments, Configuration{}
            .setTitle("Monopticon")
            .setWindowFlags(Configuration::WindowFlag::Borderless|Configuration::WindowFlag::Resizable)
            .setSize(Vector2i{1400,1000}),
            GLConfiguration{}.setSampleCount(16)},
        _framebuffer{GL::defaultFramebuffer.viewport()}
{
    std::cout << "Waiting for broker connection" << std::endl;

    uint16_t listen_port = 9999;
    std::string addr = "127.0.0.1";
    auto res = _ep.listen(addr, listen_port);
    if (res == 0) {
        std::cout << "Could not listen on: ";
        std::cout << addr << ":" << listen_port << std::endl;
        exit(1);
    } else {
        std::cout << "Endpoint listening on: ";
        std::cout << addr << ":" << listen_port << std::endl;
    }


    Util::print_peer_subs();



    /* Global renderer configuration */
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);

    _color.setStorage(GL::RenderbufferFormat::RGBA8, GL::defaultFramebuffer.viewport().size());
    _objectId.setStorage(GL::RenderbufferFormat::R32UI, GL::defaultFramebuffer.viewport().size());
    _depth.setStorage(GL::RenderbufferFormat::DepthComponent24, GL::defaultFramebuffer.viewport().size());
    _framebuffer.attachRenderbuffer(GL::Framebuffer::ColorAttachment{0}, _color)
                .attachRenderbuffer(GL::Framebuffer::ColorAttachment{1}, _objectId)
                .attachRenderbuffer(GL::Framebuffer::BufferAttachment::Depth, _depth)
                .mapForDraw({{Figure::PhongIdShader::ColorOutput, GL::Framebuffer::ColorAttachment{0}},
                            {Figure::PhongIdShader::ObjectIdOutput, GL::Framebuffer::ColorAttachment{1}}});
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
        .setProjectionMatrix(Matrix4::perspectiveProjection(50.0_degf, 1.0f, 0.001f, 100.0f))
        .setViewport(GL::defaultFramebuffer.viewport().size()); /* Drawing setup */


    {
        Trade::MeshData3D data = Primitives::uvSphereSolid(8.0f, 30.0f);
        _sphereVertices.setData(MeshTools::interleave(data.positions(0), data.normals(0)), GL::BufferUsage::StaticDraw);
        _sphereIndices.setData(MeshTools::compressIndicesAs<UnsignedShort>(data.indices()), GL::BufferUsage::StaticDraw);
        _sphere.setCount(data.indices().size())
            .setPrimitive(data.primitive())
            .addVertexBuffer(_sphereVertices, 0, Figure::PhongIdShader::Position{}, Figure::PhongIdShader::Normal{})
            .setIndexBuffer(_sphereIndices, 0, MeshIndexType::UnsignedShort);
    }

    _line_shader = Figure::ParaLineShader{};
    _phong_id_shader = Figure::PhongIdShader{};
    _bbitem_shader = Shaders::Flat3D{};
    _bbitem_shader.setColor(0x00ff00_rgbf);

    srand(time(nullptr));


    { // Known devices
        Object3D *cob = new Object3D{&_scene};
        Matrix4 scaling = Matrix4::scaling(Vector3{4.8});
        cob->transform(scaling);
        cob->rotateX(90.0_degf);
        cob->translate(Vector3{5.0f, 0.0f, 5.0f});
        new Figure::RingDrawable{*cob, 0x0000ff_rgbf, _drawables};
    }
    { // Gateways
        Object3D *coy = new Object3D{&_scene};
        Matrix4 scaling = Matrix4::scaling(Vector3{4.8});
        coy->transform(scaling);
        coy->rotateX(90.0_degf);
        coy->translate(Vector3{5.0f, 0.0f, -5.0f});
        new Figure::RingDrawable{*coy, 0xffff00_rgbf, _drawables};
    }
    { // Unknown devices
        Object3D *cor = new Object3D{&_scene};
        Matrix4 scaling = Matrix4::scaling(Vector3{4.8});
        cor->rotateX(90.0_degf);
        cor->transform(scaling);
        cor->translate(Vector3{-5.0f, 0.0f, -5.0f});
        new Figure::RingDrawable{*cor, 0xff0000_rgbf, _drawables};
    }
    { // Broadcast addrs
        Object3D *cog = new Object3D{&_scene};
        Matrix4 scaling = Matrix4::scaling(Vector3{4.8});
        cog->rotateX(90.0_degf);
        cog->transform(scaling);
        cog->translate(Vector3{-5.0f, 0.0f, 5.0f});
        new Figure::RingDrawable{*cog, 0x00ff00_rgbf, _drawables};
    }

    Vector2 p1, p2;
    {
        std::string mac_dst = "ba:dd:be:ee:ef";
        Device::Stats *d_s = createSphere(mac_dst);
        p1 = d_s->circPoint;
    }
    {
        std::string mac_dst = "ca:ff:eb:ee:ef";
        Device::Stats *d_s = createSphere(mac_dst);
        p2 = d_s->circPoint;
    }

    createLine(p1, p2, Util::L3Type::ARP);

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


    //ifaceChartMgr = DeviceChartMgr(240);
    run_sum = 0;
    frame_cnt = 0;

    setSwapInterval(1);
    setMinimalLoopPeriod(16);
    _timeline.start();

    _iface_list = Util::get_iface_list();
    _zeek_pid = "#nop";
}


void Application::parse_raw_packet(broker::zeek::Event event) {
    broker::vector parent_content = event.args();

    broker::vector *raw_pkt_hdr = broker::get_if<broker::vector>(parent_content.at(0));
    if (raw_pkt_hdr == nullptr) {
        std::cout << "rph" << std::endl;
        return;
    }
    broker::vector *l2_pkt_hdr = broker::get_if<broker::vector>(raw_pkt_hdr->at(0));
    if (l2_pkt_hdr == nullptr || l2_pkt_hdr->size() != 9) {
        std::cout << "lph" << std::endl;
        return;
    }

    std::string *mac_src = broker::get_if<std::string>(l2_pkt_hdr->at(3));
    if (mac_src == nullptr) {
        std::cout << "mac_src null" << std::endl;
        return;
    }
    std::string *mac_dst = broker::get_if<std::string>(l2_pkt_hdr->at(4));
    if (mac_dst == nullptr) {
        std::cout << "mac_dst null" << std::endl;
        return;
    }

    auto *l3_t = broker::get_if<broker::enum_value>(l2_pkt_hdr->at(8));
    if (l3_t == nullptr) {
        std::cout << "l3_i null" << std::endl;
        return;
    }

    using namespace Util;

    L3Type t;
    switch (l3_t->name.back()) {
        case L3Type::ARP:
            t = L3Type::ARP;
            break;
        case L3Type::IPV4:
            t = L3Type::IPV4;
            break;
        case L3Type::IPV6:
            t = L3Type::IPV6;
            break;
        default:
            t = L3Type::UNKNOWN;
    }

    Device::Stats *d_s;

    auto search = _device_map.find(*mac_src);
    if (search == _device_map.end()) {
        d_s = createSphere(*mac_src);
        _device_map.insert(std::make_pair(*mac_src, d_s));
    } else {
        d_s = search->second;
    }
    d_s->num_pkts_sent += 1;
    d_s->health = 60*30;
    Vector2 p1 = d_s->circPoint;

    auto search_dst = _device_map.find(*mac_dst);
    if (search_dst == _device_map.end()) {
        d_s = createSphere(*mac_dst);
    } else {
        d_s = search_dst->second;
    }
    d_s->num_pkts_recv += 1;
    d_s->health = 60*30;
    Vector2 p2 = d_s->circPoint;

    createLine(p1, p2, t);
}

void Application::deselectDevice() {
    if (_selectedDevice != nullptr) {
        _selectedDevice->setSelected(false);

        if (_selectedDevice->_highlightedDrawable != nullptr) {
            delete _selectedDevice->_highlightedDrawable;
            _selectedDevice->_highlightedDrawable = nullptr;
        }

        _selectedDevice = nullptr;
    }
}

void Application::highlightDevice(Device::Stats *d_s) {
    Object3D *o = new Object3D{&_scene};

    Matrix4 scaling = Matrix4::scaling(Vector3{2.5});
    o->transform(scaling);
    o->translate(Vector3{d_s->circPoint.x(), 0.0f, d_s->circPoint.y()});

    d_s->_highlightedDrawable = new Figure::UnitBoardDrawable{*o,
                                                               _bbitem_shader,
                                                               _billboard_drawables,
                                                               0x00ff00_rgbf};
}

void Application::deviceClicked(Device::Stats *d_s) {
    deselectDevice();

    d_s->setSelected(true);
    _selectedDevice = d_s;

    highlightDevice(d_s);
}

// TODO position create sphere in a specific ring
Device::Stats* Application::createSphere(const std::string mac) {
    Object3D* o = new Object3D{&_scene};

    Vector2 v = 4.0f*Util::randCirclePoint();
    Vector2 z = Util::randOffset(5.0f);
    v = v + z;

    o->translate({v.x(), 0.0f, v.y()});

    UnsignedByte id = static_cast<UnsignedByte>(_device_objects.size()+1);

    Color3 c = 0xa5c9ea_rgbf;
    Figure::DeviceDrawable *dev = new Figure::DeviceDrawable{
        id,
        *o,
        _phong_id_shader,
        c,
        _sphere,
        Matrix4::scaling(Vector3{0.25f}),
        _drawables};


    Device::Stats* d_s = new Device::Stats{mac, v, dev};
    dev->_deviceStats = d_s;

    _device_objects.push_back(dev);
    _device_map.insert(std::make_pair(mac, d_s));

    return d_s;
}


void Application::createLine(Vector2 a, Vector2 b, Util::L3Type t) {
    Object3D* line = new Object3D{&_scene};
    Vector3 a3 = Vector3{a.x(), 0.0f, a.y()};
    Vector3 b3 = Vector3{b.x(), 0.0f, b.y()};

    using namespace Util;

    Color3 c;
    switch (t) {
        case L3Type::ARP:
            c = 0xffff00_rgbf;
            break;
        case L3Type::IPV4:
            c = 0x00ffff_rgbf;
            break;
        case L3Type::IPV6:
            c = 0x007777_rgbf;
            break;
        default:
            c = 0xff0000_rgbf;
    }

    auto *pl = new Figure::PacketLineDrawable{*line, _line_shader, a3, b3, _drawables, c};
    _packet_line_queue.insert(pl);
}


void Application::drawEvent() {
    // Process all messages from status_subscriber before doing anything
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

    int event_cnt = 0;
    // Read and parse packets
    for (auto msg : _subscriber.poll()) {
        event_cnt++;
        if (event_cnt < 500) {
            broker::topic topic = broker::get_topic(msg);
            broker::zeek::Event event = broker::get_data(msg);
            std::cout << "received on topic: " << topic << " event: " << event.args() << std::endl;
            if (event.name().compare("raw_packet_event")) {
                    parse_raw_packet(event);
            } else {
                std::cout << "Unhandled Event: " << event.name() << std::endl;
            }
        } else {
            // This an overflow handler that drops n packets from the visualization
            // before moving to rendering
            if (event_cnt > 1000) {
                break;
            }
        }
    }
    // Update Iface packet statistics
    frame_cnt ++;
    ifaceChartMgr.push(static_cast<float>(event_cnt));

    if (frame_cnt % 15 == 0) {
        ifaceLongChartMgr.push(static_cast<float>(run_sum));
        run_sum = 0;
        frame_cnt = 0;

        Util::print_peer_subs();
    } else {
        run_sum += event_cnt;
    }

    if (frame_cnt % 60 == 0) {
        _iface_list = Util::get_iface_list();
    }

    // TODO TODO TODO TODO
    // Expire devices that haven't communicated in awhile
    // TODO TODO TODO TODO
    // steps
    // 0: add shader uniform to expire device
    // 1: refactor _device id vector to map of ids
    // 2: create _expired member and update phongid draw
    // 3: remove expired devices

    // Remove packet_lines that have expired for the queue
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

    // Actually draw things
    /* Draw to custom framebuffer */
    _framebuffer
        .clearColor(0, _clearColor)
        .clearColor(1, Vector4ui{})
        .clearDepth(1.0f)
        .bind();
    _camera->draw(_drawables);

    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);

    _camera->draw(_billboard_drawables);

    /* Bind the main buffer back */
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth)
        .bind();

    GL::Renderer::setClearColor(_clearColor);

    /* Blit color to window framebuffer */
    _framebuffer.mapForRead(GL::Framebuffer::ColorAttachment{0});
    GL::AbstractFramebuffer::blit(_framebuffer, GL::defaultFramebuffer,
        {{}, _framebuffer.viewport().size()}, GL::FramebufferBlit::Color);

    _imgui.newFrame();

    ImGui::SetNextWindowSize(ImVec2(315, 215), ImGuiCond_Always);
    ImGui::Begin("Tap Status");

    static bool peer_connected = false;
    if (!peer_connected && _iface_list.size() > 0) {
        if (ImGui::Button("Connect", ImVec2(80, 20))) {
            std::string s = "monopt_iface_proto launch ";
            std::string cmd = s.append(_iface_list.at(0));
            _zeek_pid = Util::exec_output(cmd);
            std::cout << "Launched subprocess with pid: " << _zeek_pid << std::endl;
            peer_connected = true;
        }
    } else {
        if (ImGui::Button("Disconnect", ImVec2(80, 20))) {
            std::string s = "monopt_iface_proto sstop ";
            auto cmd = s.append(_zeek_pid);
            int r = std::system(cmd.c_str());
            if (r != 0) {
                std::cout << "Listener shutdown failed" << std::endl;
            }
            std::cout << "Disconnected" << std::endl;
            peer_connected = false;
        }
    }

    int offset = 100;
    for (auto it = _iface_list.begin(); it != _iface_list.end(); it++) {
        ImGui::SameLine(offset);
        offset += 100;
        auto green = ImVec4(0,1,0,1);
        if (_iface_list.begin() == it) {
            ImGui::TextColored(green, (*it).c_str(), ImVec2(80, 20));
        } else {
            ImGui::Text((*it).c_str(), ImVec2(80, 20));
        }

    }

    ImGui::Text("App average %.3f ms/frame (%.1f FPS)",
        1000.0/Magnum::Double(ImGui::GetIO().Framerate), Magnum::Double(ImGui::GetIO().Framerate));

    ImGui::Separator();
    ifaceChartMgr.draw();
    ImGui::Separator();
    ifaceLongChartMgr.draw();
    ImGui::Separator();

    ImGui::End();

    ImGui::SetNextWindowSize(ImVec2(315, 215), ImGuiSetCond_Once);
    ImGui::Begin("Heads Up Display");

    if (ImGui::Button("Watch", ImVec2(80,20))) {
        std::cout << "clicked watch" << std::endl;

        if (_selectedDevice != nullptr && _selectedDevice->_windowMgr == nullptr) {
            Device::WindowMgr *dwm = new Device::WindowMgr(_selectedDevice);
            _selectedDevice->_windowMgr = dwm;
            _inspected_device_window_list.push_back(dwm);
        } else {
            std::cout << "ref to window already exists" << std::endl;
        }
    }

    ImGui::SameLine(100.0f);
    if (!_orbit_toggle) {
        if (ImGui::Button("Start Orbit", ImVec2(90,20))) {
               /* Rotate the camera on an orbit */
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
        if (ImGui::Button(b, ImVec2(200, 18))) {
            deviceClicked(d_s);
        }
        ImGui::SameLine(5.0f);
        d_s->renderText();
    }

    ImGui::EndChild();

    ImGui::End();

    // Render custom windows chosen by user
    for (auto it = _inspected_device_window_list.begin(); it != _inspected_device_window_list.end(); ++it) {
        Device::WindowMgr *dwm = *it;
        dwm->draw();
    }

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


void Application::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});

    _imgui.relayout(Vector2{event.windowSize()}/event.dpiScaling(),
        event.windowSize(), event.framebufferSize());
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

    if(event.button() != MouseEvent::Button::Left) return;

    _previousMousePosition = _mousePressPosition = event.position();

    event.setAccepted();
}


void Application::mouseReleaseEvent(MouseEvent& event) {
    if(_imgui.handleMouseReleaseEvent(event)) return;

    if(event.button() != MouseEvent::Button::Left || _mousePressPosition != event.position()) return;

    /* Read object ID at given click position (framebuffer has Y up while windowing system Y down) */
    _framebuffer.mapForRead(GL::Framebuffer::ColorAttachment{1});
    Image2D data = _framebuffer.read(
        Range2Di::fromSize({event.position().x(), _framebuffer.viewport().sizeY() - event.position().y() - 1}, {1, 1}),
        {PixelFormat::R32UI});


    deselectDevice();
    UnsignedByte id = data.data<UnsignedByte>()[0];
    if(id > 0 && id < _device_objects.size()+1) {
        Device::Stats *d_s = _device_objects.at(id-1)->_deviceStats;
        deviceClicked(d_s);
    } else {
        std::cout << "Could not find device" << std::endl;
    }

    event.setAccepted();
    redraw();
}


void Application::mouseMoveEvent(MouseMoveEvent& event) {
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


void Application::mouseScrollEvent(MouseScrollEvent& event) {
    if(_imgui.handleMouseScrollEvent(event)) return;
}


void Application::textInputEvent(TextInputEvent& event) {
    if(_imgui.handleTextInputEvent(event)) return;
}

}

Vector2 Monopticon::Util::randCirclePoint() {
    float r = rand() / (RAND_MAX/(2*Math::Constants<float>::pi()));

    float steps = 16.0f;
    float max = 2*Math::Constants<float>::pi();
    float min = 0.0f;

    float zerone = std::round((r-min)*(steps/(max-min)))/steps;
    float res = zerone*(max-min) + min;

    return Vector2{cos(res), sin(res)};
}

Vector2 Monopticon::Util::randOffset(float z) {
    int x = rand() % 2;
    int y = rand() % 2;
    Vector2 v = Vector2{x ? z : -z, y ? z : -z};
    return v;
}

void Monopticon::Util::print_peer_subs() {
    auto ts = _ep.peer_subscriptions();
    for (auto it = ts.begin(); it != ts.end(); it++) {
        auto t = *it;
        std::cout << t << std::endl;
    }
}

std::string Monopticon::Util::exec_output(std::string cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

std::vector<std::string> Monopticon::Util::get_iface_list() {
    auto v = std::vector<std::string>{};
    std::string s = Util::exec_output("monopt_iface_proto list_ifaces");
    std::stringstream ss(s);
    std::string t;
    while (std::getline(ss,t,'\n')) {
        v.push_back(t);
    }
    return v;
}


MAGNUM_APPLICATION_MAIN(Monopticon::Application)
