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

const int MSAA_CNT = 16; // Number of subpixel samples for MultiSampling Anti-Aliasing

// Layout constants
const int num_rings = 8;
const int elems_per_ring[8]{1, 4, 8, 16, 32, 64, 256, 10000};
const float ring_radii[8]{0.0f, 4.0f, 8.0f, 12.0f, 16.0f, 24.0f, 32.0f, 64.0f};

const Vector3 offset{0.0f, 1.0f, 0.0f};

// Zeek broker components
broker::endpoint _ep;
broker::subscriber _subscriber = _ep.make_subscriber({"monopt/l2", "monopt/stats"});
broker::status_subscriber _status_subscriber = _ep.make_status_subscriber(true);

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

        int parse_epoch_step(broker::zeek::Event event);
        void parse_enter_l3_addr(std::map<broker::data, broker::data> *addr_map);
        void parse_arp_table(std::map<broker::data, broker::data> *arp_table);

        void parse_stats_update(broker::zeek::Event event);
        void parse_bcast_summaries(broker::vector *dComm, Device::Stats* tran_d_s);
        void parse_single_mcast(int pos, std::string v, broker::vector *dComm, Device::Stats* tran_d_s);

        Device::Stats* createSphere(const std::string);

        Device::PrefixStats* createBroadcastPool(const std::string, Vector3);
        void createPoolHits(Device::Stats* tran_d_s, Device::PrefixStats *dp_s, Util::L2Summary sum);
        void createPoolHit(Device::PrefixStats *dp_s, Color3 c);
        Level3::Address* createIPv4Address(const std::string ipv4_addr, Vector3 pos);


        void createLines(Vector3, Vector3, Util::L3Type, int num);
        void createLine(Vector3, Vector3, Util::L3Type);

        void addDirectLabels(Device::Stats *d_s);
        void addL2ConnectL3(Vector3 a, Vector3 b);

        UnsignedByte newObjectId();
        void deselectObject();
        void objectClicked(Device::Selectable *selection);
        void highlightObject(Device::Selectable *selection);

        void DeleteEverything();

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
    std::cout << "Waiting for broker connection" << std::endl;

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
    //auto *ring = Util::createLayoutRing(_scene, _permanent_drawables, 10.0f, Vector3{1.0f, 0.0f, 1.0f});
    //ring->setMesh(Primitives::grid3DWireframe(Vector2i(10, 10)));

    Device::PrefixStats *ff_bcast = createBroadcastPool("ff", Vector3{1.0f, -4.0f, 1.0f});
    Device::PrefixStats *three_bcast = createBroadcastPool("33", Vector3{1.0f, -4.0f, -1.0f});
    Device::PrefixStats *one_bcast = createBroadcastPool("01", Vector3{-1.0f, -4.0f, 1.0f});
    Device::PrefixStats *odd_bcast = createBroadcastPool("odd", Vector3{-1.0f, -4.0f, -1.0f});

    _dst_prefix_group_map.insert(std::make_pair("ff", ff_bcast));
    _dst_prefix_group_map.insert(std::make_pair("33", three_bcast));
    _dst_prefix_group_map.insert(std::make_pair("01", one_bcast));
    _dst_prefix_group_map.insert(std::make_pair("odd", odd_bcast));
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

    ImGui::SetNextWindowSize(ImVec2(315, 245), ImGuiCond_Always);
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

                if (mac_addr.size() > 0) {
                    _listeningDevice = createSphere(mac_addr);
                    //objectClicked(_listeningDevice);

                    s = "monopt_iface_proto ipv4_addr ";
                    cmd = s.append(chosen_iface);
                    std::string ipv4_addr = Util::exec_output(cmd);

                    if (ipv4_addr.size() > 0) {
                        //_listeningDevice->updateMaps(ipv4_addr, "");
                    }
                    addDirectLabels(_listeningDevice);
                    createIPv4Address(ipv4_addr, _listeningDevice->circPoint);
                } else {
                    std::cerr << "Empty mac addr for net interface: " << chosen_iface << std::endl;
                }
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
                if (gw_mac_addr.size() > 0) {
                    _activeGateway = createSphere(gw_mac_addr);
                    //_activeGateway->updateMaps("0.0.0.0/32", "");
                    //_activeGateway->updateMaps(gw_ipv4_addr, "");

                    addDirectLabels(_activeGateway);
                    createIPv4Address(gw_ipv4_addr, _activeGateway->circPoint);
                } else {
                    std::cerr << "Empty mac addr for gateway: " << gw_ipv4_addr << std::endl;
                }
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
                std::cerr << "Listener shutdown failed" << std::endl;
            }
            std::cout << "Disconnected" << std::endl;
            peer_connected = false;

            DeleteEverything();
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

    ImGui::SetNextWindowSize(ImVec2(330, 215), ImGuiSetCond_Once);
    ImGui::Begin("Heads Up Display", nullptr, flags);

    if (ImGui::Button("Watch", ImVec2(80,20))) {
        auto *selectedDevice = dynamic_cast<Device::Stats*>(_selectedObject);

        if (selectedDevice != nullptr && selectedDevice->_windowMgr == nullptr) {

            Device::WindowMgr *dwm = new Device::WindowMgr(selectedDevice);
            selectedDevice->_windowMgr = dwm;
            _inspected_device_window_list.push_back(dwm);

            auto *obj = new Object3D{&_scene};
            dwm->_lineDrawable = new Figure::WorldScreenLink(*obj, 0xffffff_rgbf, _link_shader, _drawables);
        } else {
            std::cerr << "Error! Ref to window already exists" << std::endl;
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

    int epoch_packets_sum = 0;

    // Read and parse packets
    for (auto msg : _subscriber.poll()) {
        event_cnt++;
        if (event_cnt % inv_sample_rate == 0) {
            broker::topic topic = broker::get_topic(msg);
            broker::zeek::Event event = broker::get_data(msg);
            std::string name = to_string(topic);
            if (name.compare("monopt/l2") == 0) {
                epoch_packets_sum += parse_epoch_step(event);
            } else if (name.compare("monopt/stats") == 0) {
                parse_stats_update(event);
            } else {
                std::cerr << "Unhandled Event: " << event.name() << std::endl;
            }
            processed_event_cnt ++;
        } else {
            tot_epoch_drop += 1;
        }
        if (event_cnt % 16 == 0 && inv_sample_rate <= 16) {
            inv_sample_rate = inv_sample_rate * 2;
        }
    }

    // TODO NOTE WARNING dropping events on this side can introduce non existent mac_src
    // devices into the graphic
    if (event_cnt < 8 && inv_sample_rate > 1) {
        inv_sample_rate = inv_sample_rate/2;
    }

    if (frame_cnt % 60 == 0) {
        _iface_list = Util::get_iface_list();
    }

    ifaceChartMgr.push(static_cast<float>(epoch_packets_sum));

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
        // TODO get answers from xenomit
        dp_s->contacts = c;
    }

    return event_cnt;
}


int Application::parse_epoch_step(broker::zeek::Event event) {
    broker::vector parent_content = event.args();

    broker::vector *wrapper = broker::get_if<broker::vector>(parent_content.at(0));
    if (wrapper == nullptr) {
        std::cerr << "wrapper" << std::endl;
        return 0;
    }

    broker::set *enter_l2_devices = broker::get_if<broker::set>(wrapper->at(0));
    if (enter_l2_devices == nullptr) {
        std::cerr << "enter_l2_devices" << std::endl;
        return 0;
    }

    for (auto it = enter_l2_devices->begin(); it != enter_l2_devices->end(); it++) {
        auto *mac_src = broker::get_if<std::string>(*it);
        if (mac_src == nullptr) {
            std::cerr << "mac_src e_l2_dev" << std::endl;
            return 0;
        }

        Device::Stats *d_s = createSphere(*mac_src);
        _device_map.insert(std::make_pair(*mac_src, d_s));
        addDirectLabels(d_s);
    }

    std::map<broker::data, broker::data> *l2_dev_comm = broker::get_if<broker::table>(wrapper->at(1));
    if (l2_dev_comm == nullptr) {
        std::cerr << "l2_dev_comm" << std::endl;
        return 0;
    }

    int pkt_tot = 0;

    for (auto it2 = l2_dev_comm->begin(); it2 != l2_dev_comm->end(); it2++) {
        auto pair = *it2;

        auto *mac_src = broker::get_if<std::string>(pair.first);
        if (mac_src == nullptr) {
            std::cerr << "mac_src e_l2_dev" << std::endl;
            continue;
        }

        Device::Stats *tran_d_s;
        auto search = _device_map.find(*mac_src);
        if (search != _device_map.end()) {
            tran_d_s = search->second;
        } else {
            std::cerr << "tran_d_s not found! " << *mac_src << std::endl;
            continue;
        }

        auto *dComm = broker::get_if<broker::vector>(pair.second);
        if (dComm == nullptr) {
            std::cerr << "dComm" <<  std::endl;
            continue;
        }

        parse_bcast_summaries(dComm, tran_d_s);

        std::map<broker::data, broker::data> *tx_summary = broker::get_if<broker::table>(dComm->at(1));
        if (tx_summary == nullptr) {
            std::cerr << "tx_summary" <<  std::endl;
            continue;
        }

        for (auto it3 = tx_summary->begin(); it3 != tx_summary->end(); it3++) {
            auto comm_pair = *it3;
            auto *mac_dst = broker::get_if<std::string>(comm_pair.first);
            if (mac_dst == nullptr) {
                std::cerr << "mac_dst tx_summary:" << mac_src << std::endl;
                continue;
            }

            Device::Stats *recv_d_s;
            auto search = _device_map.find(*mac_dst);
            if (search != _device_map.end()) {
                recv_d_s = search->second;
            } else {
                std::cerr << "recv_d_s not found! " << *mac_dst << std::endl;
                continue;
            }

            auto *l2summary = broker::get_if<broker::vector>(comm_pair.second);
            if (l2summary == nullptr) {
                std::cerr << "l2summary" << mac_src << std::endl;
                continue;
            }
            Util::L2Summary struct_l2 = Util::parseL2Summary(l2summary);

            Vector3 p1 = tran_d_s->circPoint;
            Vector3 p2 = recv_d_s->circPoint;

            createLines(p1, p2, Util::L3Type::IPV4, struct_l2.ipv4_cnt);
            createLines(p1, p2, Util::L3Type::IPV6, struct_l2.ipv6_cnt);
            createLines(p1, p2, Util::L3Type::ARP, struct_l2.arp_cnt);
            createLines(p1, p2, Util::L3Type::UNKNOWN, struct_l2.unknown_cnt);

            int dev_tot = Util::SumTotal(struct_l2);
            tran_d_s->num_pkts_sent += dev_tot;
            recv_d_s->num_pkts_recv += dev_tot;

            pkt_tot += dev_tot;
        }
    }

    std::map<broker::data, broker::data> *enter_l2_ipv4_addr_src = broker::get_if<broker::table>(wrapper->at(2));
    if (enter_l2_ipv4_addr_src == nullptr) {
        std::cerr << "l2_ipv4_addr_src" << std::endl;
        return 0;
    }
    parse_enter_l3_addr(enter_l2_ipv4_addr_src);

    std::map<broker::data, broker::data> *enter_arp_table = broker::get_if<broker::table>(wrapper->at(3));
    if (enter_l2_ipv4_addr_src == nullptr) {
        std::cerr << "enter_arp_table" << std::endl;
        return 0;
    }
    parse_arp_table(enter_arp_table);


    return pkt_tot;
}

void Application::parse_enter_l3_addr(std::map<broker::data, broker::data> *addr_map) {
     for (auto it = addr_map->begin(); it != addr_map->end(); it++) {
        auto pair = *it;
        auto *mac_src = broker::get_if<std::string>(pair.first);
        if (mac_src == nullptr) {
            std::cerr << "mac_src l2_ipv4_addr:" << std::endl;
            continue;
        }

        Device::Stats *tran_d_s;
        auto search = _device_map.find(*mac_src);
        if (search != _device_map.end()) {
            tran_d_s = search->second;
        } else {
            std::cerr << "tran_d_s l2_ipv4_addr not found! " << *mac_src << std::endl;
            continue;
        }

        auto *ip_addr_src = broker::get_if<broker::address>(pair.second);
        if (ip_addr_src == nullptr) {
            std::cerr << "ip_addr_src l2_ipv4_addr:" << *mac_src << std::endl;
            continue;
        }

        std::string s = to_string(*ip_addr_src);

        createIPv4Address(s, tran_d_s->circPoint);
    }
}


void Application::parse_arp_table(std::map<broker::data, broker::data> *arp_table) {
    for (auto it = arp_table->begin(); it != arp_table->end(); it++) {
        auto pair = *it;
        auto *mac_src = broker::get_if<std::string>(pair.first);
        if (mac_src == nullptr) {
            std::cerr << "mac_src arp_table:" << std::endl;
            continue;
        }

        Device::Stats *tran_d_s;
        auto search = _device_map.find(*mac_src);
        if (search != _device_map.end()) {
            tran_d_s = search->second;
        } else {
            std::cerr << "mac_src arp_table not found! " << *mac_src << std::endl;
            continue;
        }

        std::map<broker::data, broker::data> *src_table = broker::get_if<broker::table>(pair.second);
        if (src_table == nullptr) {
            std::cerr << "src_table arp_table:" << *mac_src << std::endl;
            continue;
        }

        for (auto it2 = src_table->begin(); it2 != src_table->end(); it2++) {
            auto pair2 = *it2;
            auto *mac_dst = broker::get_if<std::string>(pair2.first);
            if (mac_dst == nullptr) {
                std::cerr << "mac_dst arp_table:" << std::endl;
                continue;
            }

            Device::Stats *recv_d_s;
            auto search2 = _device_map.find(*mac_dst);
            if (search2 != _device_map.end()) {
                recv_d_s = search2->second;
            } else {
                std::cerr << "mac_dst arp_table not found! " << *mac_dst << std::endl;
                continue;
            }

            auto *ip_addr_dst = broker::get_if<broker::address>(pair2.second);
            if (ip_addr_dst == nullptr) {
                std::cerr << "ip_addr_dst arp_table:" << *mac_dst << std::endl;
                continue;
            }

            addL2ConnectL3(tran_d_s->circPoint, recv_d_s->circPoint+offset);
            addL2ConnectL3(recv_d_s->circPoint+offset, tran_d_s->circPoint);
        }

    }

}


UnsignedByte Application::newObjectId() {
    return static_cast<UnsignedByte>(_selectable_objects.size());
}


void Application::parse_single_mcast(int pos, std::string v, broker::vector *dComm, Device::Stats* tran_d_s) {
    Util::L2Summary sum;
    Device::PrefixStats *dp_s = nullptr;

    auto *bcast_val = broker::get_if<broker::vector>(dComm->at(pos));
    if (bcast_val != nullptr) {
        sum = Util::parseL2Summary(bcast_val);
        dp_s = _dst_prefix_group_map.at(v);
        createPoolHits(tran_d_s, dp_s, sum);
    }
}


void Application::parse_bcast_summaries(broker::vector *dComm, Device::Stats* tran_d_s) {
    parse_single_mcast(2, "ff", dComm, tran_d_s);
    parse_single_mcast(3, "33", dComm, tran_d_s);
    parse_single_mcast(4, "01", dComm, tran_d_s);
    parse_single_mcast(5, "odd", dComm, tran_d_s);
}


void Application::parse_stats_update(broker::zeek::Event event) {
    broker::vector parent_content = event.args();

    broker::vector *wrapper = broker::get_if<broker::vector>(parent_content.at(0));
    if (wrapper == nullptr) {
        std::cerr << "stat wrapper" << std::endl;
        return;
    }

    broker::count *mem_usage = broker::get_if<broker::count>(wrapper->at(2));
    if (mem_usage == nullptr) {
        std::cerr << "stat mem" << std::endl;
        return;
    }

    broker::count *pkts_proc = broker::get_if<broker::count>(wrapper->at(3));
    if (pkts_proc == nullptr) {
        std::cerr << "stat pkts_proc" << std::endl;
        return;
    }

    // Update iface packet statistics
    ifaceLongChartMgr.push(static_cast<float>(*pkts_proc));

    broker::count *pkts_drop = broker::get_if<broker::count>(wrapper->at(5));
    if (pkts_drop == nullptr) {
        std::cerr << "stat pkts_drop" << std::endl;
        return;
    }
    tot_pkt_drop += (*pkts_drop);

    broker::timespan *pkt_lag = broker::get_if<broker::timespan>(wrapper->at(7));
    if (pkt_lag == nullptr) {
        std::cerr << "stat pkt_lag" << std::endl;
        return;
    }

    curr_pkt_lag = *pkt_lag;
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


void Application::highlightObject(Device::Selectable *selection) {


}


void Application::objectClicked(Device::Selectable *selection) {
    deselectObject();
    _selectedObject = selection;

    //Object3D *o = selection->getObj();

    //Level3::Address *a = dynamic_cast<Level3::Address*>(selection);
    selection->addHighlight(_bbitem_shader, _billboard_drawables);

    _selectedObject = selection;

}


Device::Stats* Application::createSphere(const std::string mac) {
    Object3D* o = new Object3D{&_scene};

    int num_objs = _device_map.size();

    // Sequentially add elements to rings of increasing radius
    int j = num_objs;
    int pos = 0;
    int ring = 0;

    for (int i = 0; i < num_rings; i++) {
        ring = i;
        if (j < elems_per_ring[i]) {
            pos = j;
            if (pos == 0) {
                //Util::createLayoutRing(_scene, _drawables, ring_radii[i], Vector3{0.0f, 0.0f, 0.0f});
            }
            break;
        }
        j = j - elems_per_ring[i];
    }

    float ring_radius = ring_radii[ring];
    Vector2 v = ring_radius*Util::paramCirclePoint(elems_per_ring[ring], pos);
    Vector3 w = Vector3{v.x(), 0.0f, v.y()};

    o->transform(Matrix4::scaling(Vector3{0.25f}));
    o->translate(w);


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


    Device::Stats* d_s = new Device::Stats{mac, w, dev};
    dev->_deviceStats = d_s;

    _selectable_objects.push_back(d_s);
    _device_map.insert(std::make_pair(mac, d_s));

    return d_s;
}


Device::PrefixStats* Application::createBroadcastPool(const std::string mac_prefix, Vector3 pos) {
    auto *ring = Util::createLayoutRing(_scene, _permanent_drawables, 1.0f, pos);
    Device::PrefixStats* dp_s = new Device::PrefixStats{mac_prefix, pos, ring};

    return dp_s;
}


Level3::Address* Application::createIPv4Address(const std::string ipv4_addr, Vector3 pos) {
    auto t = pos+offset;

    Utility::Debug{} << t;

    Object3D* g = new Object3D{&_scene};
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

    Object3D *obj = new Object3D{&_scene};
    auto scaling = Matrix4::scaling(Vector3{0.10f});
    obj->transform(scaling);

    auto p = t+Vector3(0.0f, 0.5f, 0.0f);
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


void Application::createPoolHits(Device::Stats* tran_d_s, Device::PrefixStats *dp_s, Util::L2Summary sum) {
    using namespace Monopticon::Util;

    if (sum.ipv4_cnt > 0) {
        createPoolHit(dp_s, typeColor(L3Type::IPV4));
        createLine(tran_d_s->circPoint, dp_s->_position, L3Type::IPV4);
    }
    if (sum.ipv6_cnt > 0) {
        createPoolHit(dp_s, typeColor(L3Type::IPV6));
        createLine(tran_d_s->circPoint, dp_s->_position, L3Type::IPV6);
    }
    if (sum.arp_cnt > 0) {
        createPoolHit(dp_s, typeColor(L3Type::ARP));
        createLine(tran_d_s->circPoint, dp_s->_position, L3Type::ARP);
    }
    if (sum.unknown_cnt > 0) {
        createPoolHit(dp_s, typeColor(L3Type::UNKNOWN));
        createLine(tran_d_s->circPoint, dp_s->_position, L3Type::UNKNOWN);
    }

    tran_d_s->num_pkts_sent += Util::SumTotal(sum);
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


void Application::DeleteEverything() {
    // Delete this stuff. . . .
    /*
    for (int i = 0; i < _drawables.size(); ) {
        auto *o = _drawables[i];
        _drawables.remove(i);
        delete o;

    }*/
    //delete _selectable_drawables;
    //delete _billboard_drawables;
    //delete _text_drawables;

    _selectable_objects.clear();
    _packet_line_queue.clear();
    _inspected_device_window_list.clear();

    _device_map.clear();
    //_dst_prefix_group_map.clear();
    _prefix_group_map.clear();

    // re-initialize application state;
    _selectedObject = nullptr;
    _listeningDevice = nullptr;
    _activeGateway = nullptr;

    ring_level = 0;
    pos_in_ring = 0;

    _orbit_toggle = false;

    // TODO the objects under these values are not destroyed.
    // Reset scene state
    _drawables = SceneGraph::DrawableGroup3D{};
    _selectable_drawables = SceneGraph::DrawableGroup3D{};
    _billboard_drawables = SceneGraph::DrawableGroup3D{};
    _text_drawables = SceneGraph::DrawableGroup3D{};

    redraw();
}


void Application::drawEvent() {
    int event_cnt = processNetworkEvents();

    draw3DElements();

    drawTextElements();

    drawIMGuiElements(event_cnt);

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

    if(event.button() != MouseEvent::Button::Left) return;

    _previousMousePosition = _mousePressPosition = event.position();

    event.setAccepted();
}


void Application::mouseReleaseEvent(MouseEvent& event) {
    if(_imgui.handleMouseReleaseEvent(event)) return;

    if(event.button() != MouseEvent::Button::Left || _mousePressPosition != event.position()) return;

    /* Read object ID at given click position (framebuffer has Y up while windowing system Y down) */
    _objselect_framebuffer.mapForRead(GL::Framebuffer::ColorAttachment{0});
    Image2D data = _objselect_framebuffer.read(
        Range2Di::fromSize({event.position().x(), _objselect_framebuffer.viewport().sizeY() - event.position().y() - 1}, {1, 1}),
        {PixelFormat::R32UI});

    deselectObject();
    UnsignedByte id = data.data<UnsignedByte>()[0];
    unsigned short i = static_cast<unsigned short>(id);
    if(i > 0 && i < _selectable_objects.size()+1) {
        Device::Selectable *selection = _selectable_objects.at(i-1);

        objectClicked(selection);

        _cameraRig->resetTransformation();
        _cameraRig->translate(selection->getTranslation());
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
