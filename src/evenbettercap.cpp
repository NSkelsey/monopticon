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

void print_peer_subs();

using namespace Magnum;
using namespace Math::Literals;


class Application: public Platform::Application {
    public:
        explicit Application(const Arguments& arguments);

        void prepare3DFont();

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

        void parse_raw_packet(broker::zeek::Event event);
        Device::Stats* createSphere(const std::string);

        Device::PrefixStats* createBroadcastPool(const std::string, Vector3);
        void createPoolHit(Device::PrefixStats *dp_s, Color3 c);


        void createLine(Vector3, Vector3, Util::L3Type);

        void addDirectLabels(Device::Stats *d_s);

        void deselectDevice();
        void deviceClicked(Device::Stats *d_s);
        void highlightDevice(Device::Stats *d_s);

    private:
        // UI fields
        ImGuiIntegration::Context _imgui{NoCreate};

        // Graphic fields
        GL::Mesh _sphere{}, _poolCircle{NoCreate};
        Color4 _clearColor = 0x002b36_rgbf;
        Color3 _pickColor = 0xffffff_rgbf;

        Figure::PhongIdShader _phong_id_shader;
        Figure::ParaLineShader _line_shader;
        Figure::PoolShader _pool_shader;
        Figure::WorldLinkShader _link_shader;
        Shaders::Flat3D _bbitem_shader;

        Scene3D _scene;
        SceneGraph::Camera3D* _camera;
        SceneGraph::DrawableGroup3D _drawables;
        SceneGraph::DrawableGroup3D _billboard_drawables;
        SceneGraph::DrawableGroup3D _text_drawables;
        Timeline _timeline;

        Object3D *_cameraRig, *_cameraObject;

        GL::Framebuffer _framebuffer;
        Vector2i _previousMousePosition, _mousePressPosition;
        GL::Renderbuffer _color, _objectId, _depth;

        GL::Buffer _sphereVertices, _sphereIndices;

        // Font graphics fields
        PluginManager::Manager<Text::AbstractFont> _manager;
        Containers::Pointer<Text::AbstractFont> _font;

        Text::DistanceFieldGlyphCache _glyphCache;
        Shaders::DistanceFieldVector3D _text_shader;

        // Scene objects
        std::vector<Figure::DeviceDrawable*> _device_objects{};
        std::set<Figure::PacketLineDrawable*> _packet_line_queue{};

        std::map<std::string, Device::Stats*> _device_map{};
        std::map<std::string, Device::PrefixStats*> _dst_prefix_group_map{};
        std::map<std::string, Device::PrefixStats*> _prefix_group_map{};

        std::vector<Device::WindowMgr*> _inspected_device_window_list{};

        Device::Stats* _selectedDevice{nullptr};
        Device::Stats* _listeningDevice{nullptr};
        Device::Stats* _activeGateway{nullptr};

        // Custom ImGui interface components
        Device::ChartMgr ifaceChartMgr{240, 1.5f};
        Device::ChartMgr ifaceLongChartMgr{300, 3.0f};

        int num_rings = 8;
        int elems_per_ring[8]{1, 4, 8, 16, 32, 64, 256, 10000};
        float ring_radii[8]{0.0f, 4.0f, 8.0f, 12.0f, 16.0f, 24.0f, 32.0f, 64.0f};

        int ring_level{0};
        int pos_in_ring{0};

        int run_sum;
        int frame_cnt;

        bool _orbit_toggle{false};

        int inv_sample_rate{1};

        std::vector<std::string> _iface_list;
        std::string _zeek_pid;
};


Application::Application(const Arguments& arguments):
        Platform::Application{arguments, Configuration{}
            .setTitle("Monopticon")
            .setWindowFlags(Configuration::WindowFlag::Borderless|Configuration::WindowFlag::Resizable)
            .setSize(Vector2i{1400,1000}),
            GLConfiguration{}.setSampleCount(16)},
        _framebuffer{GL::defaultFramebuffer.viewport()},
        _glyphCache(Vector2i(2048), Vector2i(512), 22)
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

    print_peer_subs();

    /* Global renderer configuration */
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
        .translate(Vector3::zAxis(20.0f))
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

    _poolCircle = MeshTools::compile(Primitives::circle3DWireframe(20));

    _line_shader = Figure::ParaLineShader{};
    _phong_id_shader = Figure::PhongIdShader{};
    _pool_shader = Figure::PoolShader{};
    _link_shader = Figure::WorldLinkShader{};

    _bbitem_shader = Shaders::Flat3D{};
    _bbitem_shader.setColor(0x00ff00_rgbf);

    srand(time(nullptr));

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


    prepare3DFont();

    {
        Util::createLayoutRing(_scene, _drawables, 2.5f, Vector3{0.0f, -4.0f, 0.0f});
        Device::PrefixStats *ff_bcast = createBroadcastPool("ff", Vector3{1.0f, -4.0f, 1.0f});
        Device::PrefixStats *three_bcast = createBroadcastPool("33", Vector3{1.0f, -4.0f, -1.0f});
        Device::PrefixStats *one_bcast = createBroadcastPool("01", Vector3{-1.0f, -4.0f, 1.0f});
        Device::PrefixStats *odd_bcast = createBroadcastPool("odd", Vector3{-1.0f, -4.0f, -1.0f});

        _dst_prefix_group_map.insert(std::make_pair("ff", ff_bcast));
        _dst_prefix_group_map.insert(std::make_pair("33", three_bcast));
        _dst_prefix_group_map.insert(std::make_pair("01", one_bcast));
        _dst_prefix_group_map.insert(std::make_pair("odd", odd_bcast));
    }
    {
        Device::PrefixStats *ap_arpers = createBroadcastPool("00:04", Vector3{1.0f, 4.0f, 1.0f});
        ap_arpers->ring->setMesh(Primitives::planeWireframe());

        _prefix_group_map.insert(std::make_pair("00:04", ap_arpers));
    }
}

void Application::prepare3DFont() {
    /* Load FreeTypeFont plugin */
    _font = _manager.loadAndInstantiate("FreeTypeFont");
    if(!_font) std::exit(1);

    /* Open the font and fill glyph cache */
    Utility::Resource rs("monopticon");
    if(!_font->openData(rs.getRaw("src/assets/DejaVuSans.ttf"), 110.0f)) {
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
}

void Application::draw3DElements() {
    // Actually draw things to a custom framebuffer
    _framebuffer
        .clearColor(0, _clearColor)
        .clearColor(1, Vector4ui{})
        .clearDepth(1.0f)
        .bind();

    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

    _camera->draw(_drawables);
    _camera->draw(_billboard_drawables);

    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);

    /* Bind the main buffer back */
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth)
        .bind();

    GL::Renderer::setClearColor(_clearColor);

    /* Blit color to window framebuffer */
    _framebuffer.mapForRead(GL::Framebuffer::ColorAttachment{0});
    GL::AbstractFramebuffer::blit(_framebuffer, GL::defaultFramebuffer,
        {{}, _framebuffer.viewport().size()}, GL::FramebufferBlit::Color);

}

void Application::drawIMGuiElements(int event_cnt) {
    _imgui.newFrame();

    ImGui::SetNextWindowSize(ImVec2(315, 225), ImGuiCond_Always);
    auto flags = ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoScrollbar;
    ImGui::Begin("Tap Status", nullptr, flags);

    static bool peer_connected = false;
    if (!peer_connected && _iface_list.size() > 0) {
        if (ImGui::Button("Connect", ImVec2(80, 20))) {
            std::string chosen_iface = _iface_list.at(0);
            std::string s, cmd;

            if (_listeningDevice == nullptr) {
                s = "monopt_iface_proto mac_addr ";
                cmd = s.append(chosen_iface);
                std::string mac_addr = Util::exec_output(cmd);

                _listeningDevice = createSphere(mac_addr);
                deviceClicked(_listeningDevice);

                s = "monopt_iface_proto ipv4_addr ";
                cmd = s.append(chosen_iface);
                std::string ipv4_addr = Util::exec_output(cmd);

                if (ipv4_addr.size() > 0) {
                    _listeningDevice->updateMaps(ipv4_addr, "");
                }
                addDirectLabels(_listeningDevice);
            }

            s = "monopt_iface_proto gateway_ipv4_addr ";
            cmd = s.append(chosen_iface);
            std::string gw_ipv4_addr = Util::exec_output(cmd);

            if (_activeGateway == nullptr && gw_ipv4_addr.size() > 0) {
                s = "monopt_iface_proto gateway_mac_addr ";
                cmd = s.append(chosen_iface)
                       .append(" ")
                       .append(gw_ipv4_addr);
                std::string gw_mac_addr = Util::exec_output(cmd);

                _activeGateway = createSphere(gw_mac_addr);
                _activeGateway->updateMaps("0.0.0.0/32", "");
                _activeGateway->updateMaps(gw_ipv4_addr, "");

                addDirectLabels(_activeGateway);
            }

            s = "monopt_iface_proto launch ";
            cmd = s.append(chosen_iface);
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
    ImGui::Text("Sample rate %.3f SPS event cnt %d",
        1.0/inv_sample_rate, event_cnt);

    ImGui::Separator();
    ifaceChartMgr.draw();
    ImGui::Separator();
    ifaceLongChartMgr.draw();
    ImGui::Separator();

    ImGui::End();

    ImGui::SetNextWindowSize(ImVec2(315, 215), ImGuiSetCond_Once);
    ImGui::Begin("Heads Up Display", nullptr, flags);

    if (ImGui::Button("Watch", ImVec2(80,20))) {
        if (_selectedDevice != nullptr && _selectedDevice->_windowMgr == nullptr) {
            Device::WindowMgr *dwm = new Device::WindowMgr(_selectedDevice);
            _selectedDevice->_windowMgr = dwm;
            _inspected_device_window_list.push_back(dwm);

            auto *obj = new Object3D{&_scene};
            dwm->_lineDrawable = new Figure::WorldScreenLink(*obj, 0xffffff_rgbf, _link_shader, _drawables);


        } else {
            std::cout << "Error! Ref to window already exists" << std::endl;
        }
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
        if (ImGui::InvisibleButton(b, ImVec2(200, 18))) {
            deviceClicked(d_s);
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

int Application::processNetworkEvents() {
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
    int processed_event_cnt = 0;

    // Read and parse packets
    for (auto msg : _subscriber.poll()) {
        event_cnt++;
        if (event_cnt % inv_sample_rate == 0) {
            broker::topic topic = broker::get_topic(msg);
            broker::zeek::Event event = broker::get_data(msg);
            //std::cout << "received on topic: " << topic << " event: " << event.args() << std::endl;
            if (event.name().compare("raw_packet_event")) {
                    parse_raw_packet(event);
            } else {
                std::cout << "Unhandled Event: " << event.name() << std::endl;
            }
            processed_event_cnt ++;
        }
        if (event_cnt % 256 == 0 && inv_sample_rate <= 256) {
            inv_sample_rate = inv_sample_rate * 2;
        }
    }

    if (event_cnt < 25 && inv_sample_rate > 1) {
        inv_sample_rate = inv_sample_rate/2;
    }

    // Update Iface packet statistics
    frame_cnt ++;
    ifaceChartMgr.push(static_cast<float>(event_cnt));

    if (frame_cnt % 15 == 0) {
        ifaceLongChartMgr.push(static_cast<float>(run_sum));
        run_sum = 0;
        frame_cnt = 0;

        print_peer_subs();
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
        // TODO get answers from xenomit
        dp_s->contacts = c;
    }

    return event_cnt;
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
    std::string ip_src, ip_dst;
    broker::address *ip_src_addr = nullptr;
    broker::address *ip_dst_addr = nullptr;

    switch (l3_t->name.back()) {
        case L3Type::ARP:
            t = L3Type::ARP;
            break;
        case L3Type::IPV4:
            t = L3Type::IPV4;
            {
                broker::vector *ip_pkt_hdr = broker::get_if<broker::vector>(raw_pkt_hdr->at(1));
                if (ip_pkt_hdr == nullptr || ip_pkt_hdr->size() != 8) {
                    std::cout << "ip_pkt_hdr" << std::endl;
                    break;
                }
                ip_src_addr = broker::get_if<broker::address>(ip_pkt_hdr->at(6));
                if (ip_src_addr == nullptr) {
                    std::cout << "ip_src null" << std::endl;
                    break;
                } else {
                    ip_src = broker::to_string(*ip_src_addr);
                }

                ip_dst_addr = broker::get_if<broker::address>(ip_pkt_hdr->at(6));
                if (ip_dst_addr == nullptr) {
                    std::cout << "ip_src null" << std::endl;
                    break;
                } else {
                    ip_dst = broker::to_string(*ip_dst_addr);
                }
            }

            break;
        case L3Type::IPV6:
            t = L3Type::IPV6;
            break;
        default:
            t = L3Type::UNKNOWN;
    }

    Vector3 p1;

    auto g = mac_src->substr(0,5);
    if (g == "00:04") {
        Device::PrefixStats *dp_s = _prefix_group_map.at(g);
        p1 = dp_s->_position;
    } else {
        Device::Stats *tran_d_s;

        auto search = _device_map.find(*mac_src);
        if (search == _device_map.end()) {
            tran_d_s = createSphere(*mac_src);
            _device_map.insert(std::make_pair(*mac_src, tran_d_s));
        } else {
            tran_d_s = search->second;
        }
        tran_d_s->num_pkts_sent += 1;
        tran_d_s->health = 60*30;
        p1 = tran_d_s->circPoint;

        if (ip_src_addr != nullptr && ip_dst_addr != nullptr) {
            tran_d_s->updateMaps(ip_src, *mac_dst);
            // TODO TODO TODO TODO
            // TODO TODO TODO TODO
        }

        addDirectLabels(tran_d_s);
    }

    Vector3 p2;
    Color3 c = Util::typeColor(t);

    // Check for multicast addresses
    auto s = (mac_dst->substr(0,2)).c_str();
    int l = strtol(s, nullptr, 16);
    if (l%2 == 1) {
        Device::PrefixStats *dp_s = _dst_prefix_group_map.at(std::string(s));
        if (dp_s == nullptr) {
            dp_s = _dst_prefix_group_map.at("odd");
        }
        p2 = dp_s->_position;
        createPoolHit(dp_s, c);
    } else if (mac_dst->substr(0,5) == "00:04") {
        Device::PrefixStats *dp_s = _prefix_group_map.at("00:04");
        p2 = dp_s->_position;
    } else {
        Device::Stats *recv_d_s;

        auto search_dst = _device_map.find(*mac_dst);
        if (search_dst == _device_map.end()) {
            recv_d_s = createSphere(*mac_dst);
        } else {
            recv_d_s = search_dst->second;
        }
        recv_d_s->num_pkts_recv += 1;
        recv_d_s->health = 60*30;
        p2 = recv_d_s->circPoint;
    }

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

void Application::highlightDevice(Device::Stats *d_s) {
    Object3D *o = new Object3D{&_scene};

    Matrix4 scaling = Matrix4::scaling(Vector3{2.5});

    o->transform(scaling);

    auto t = d_s->circPoint;
    o->translate(t);

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

Device::Stats* Application::createSphere(const std::string mac) {
    Object3D* o = new Object3D{&_scene};

    int num_objs = _device_objects.size();

    // Sequentially add elements to rings of increasing radius
    int j = num_objs;
    int pos = 0;
    int ring = 0;

    for (int i = 0; i < num_rings; i++) {
        ring = i;
        if (j < elems_per_ring[i]) {
            pos = j;
            if (pos == 0) {
                Util::createLayoutRing(_scene, _drawables, ring_radii[i], Vector3{0.0f, 0.0f, 0.0f});
            }
            break;
        }
        j = j - elems_per_ring[i];
    }

    float ring_radius = ring_radii[ring];
    Vector2 v = ring_radius*Util::paramCirclePoint(elems_per_ring[ring], pos);
    Vector3 w = Vector3{v.x(), 0.0f, v.y()};

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


    Device::Stats* d_s = new Device::Stats{mac, w, dev};
    dev->_deviceStats = d_s;

    _device_objects.push_back(dev);
    _device_map.insert(std::make_pair(mac, d_s));

    return d_s;
}

Device::PrefixStats* Application::createBroadcastPool(const std::string mac_prefix, Vector3 pos) {
    auto ring = Util::createLayoutRing(_scene, _drawables, 1.0f, pos);
    Device::PrefixStats* dp_s = new Device::PrefixStats{mac_prefix, pos, ring};

    return dp_s;
}

void Application::createPoolHit(Device::PrefixStats *dp_s, Color3 c) {
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

void Application::createLine(Vector3 a, Vector3 b, Util::L3Type t) {
    Object3D* line = new Object3D{&_scene};

    Color3 c = Util::typeColor(t);

    auto *pl = new Figure::PacketLineDrawable{*line, _line_shader, a, b, _drawables, c};
    _packet_line_queue.insert(pl);
}


void Application::drawEvent() {
    int event_cnt = processNetworkEvents();

    draw3DElements();

    drawTextElements();

    drawIMGuiElements(event_cnt);

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

void print_peer_subs() {
    auto ts = Monopticon::_ep.peer_subscriptions();
    for (auto it = ts.begin(); it != ts.end(); it++) {
        auto t = *it;
        std::cout << t << std::endl;
    }
}

}

MAGNUM_APPLICATION_MAIN(Monopticon::Application)
