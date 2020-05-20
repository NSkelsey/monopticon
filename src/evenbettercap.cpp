/* *
 * Monopticon displays and filters ethernet traffic in 3D
 * Copyright (C) 2019 Nick Skelsey
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * */

#include "evenbettercap.h"

namespace Monopticon {

const int MSAA_CNT = 8; // Number of subpixel samples for MultiSampling Anti-Aliasing

// Layout constants
const int num_rings = 8;
const int elems_per_ring[8]{1, 4, 8, 16, 32, 64, 256, 10000};
const float ring_radii[8]{0.0f, 4.0f, 8.0f, 12.0f, 16.0f, 24.0f, 32.0f, 64.0f};

using namespace Magnum;
using namespace Math::Literals;


class Application: public Platform::Application {
    public:
        explicit Application(const Arguments& arguments);

        void prepareDrawables();

        void drawEvent() override;
        void drawTextElements();
        void drawIMGuiElements();

        void viewportEvent(ViewportEvent& event) override;

        void keyPressEvent(KeyEvent& event) override;
        void keyReleaseEvent(KeyEvent& event) override;

        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;
        void mouseScrollEvent(MouseScrollEvent& event) override;
        void textInputEvent(TextInputEvent& event) override;

        UnsignedByte newObjectId();
        void deselectObject();
        void objectClicked(Device::Selectable *selection);
        void selectableMenuActions(Device::Selectable *selection);
        void watchSelectedDevice();

        void DeleteEverything();

        //Parse::BrokerCtx     *brokerCtx;
        Context::Graphic *gCtx;
        Context::Store    *sCtx;

    private:
        // UI fields
        ImGuiIntegration::Context _imgui{NoCreate};

        std::vector<Device::WindowMgr*> _inspected_device_window_list{};

        Device::Selectable* _selectedObject{nullptr};
        Device::Stats* _listeningDevice{nullptr};
        Device::Stats* _activeGateway{nullptr};


        // User input fields
        Vector2i _previousMousePosition, _mousePressPosition;

        // Network interface state tracking
        std::vector<std::string> _iface_list;
        std::string _chosen_iface;

        std::string _zeek_pid;

        Timeline _timeline;

        int ring_level{0};
        int pos_in_ring{0};

        int run_sum;
        int frame_cnt;

        bool _orbit_toggle{false};
        bool _openPopup{false};
};


Application::Application(const Arguments& arguments):
        Platform::Application{arguments, Configuration{}
            .setTitle("Monopticon")
            .setWindowFlags(Configuration::WindowFlag::Resizable)
            .setSize(Vector2i{1400,1000}),
            GLConfiguration{}.setSampleCount(MSAA_CNT)}
{

    /*
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
    */

    uint16_t listen_port = 9999;
    std::string addr = "127.0.0.1";

    //brokerCtx = new Parse::BrokerCtx(addr, listen_port);

    gCtx = new Context::Graphic();
    sCtx = new Context::Store();

    srand(time(nullptr));

    _imgui = ImGuiIntegration::Context(Vector2{windowSize()}/dpiScaling(),
        windowSize(), framebufferSize());

    run_sum = 0;
    frame_cnt = 0;

    setSwapInterval(1);
    //setMinimalLoopPeriod(16);
    _timeline.start();

    //_iface_list = Util::get_iface_list();
    _zeek_pid = "#nop";

    prepareDrawables();
}

static void gen_random(std::string *s, const int len) {
   static const char alphanum[] =
   "0123456789"
   "abcdef";

   for (int i = 0; i < len; ++i) {
       s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
   }
   //s[len] = 0;
}


void Application::prepareDrawables() {
    Device::PrefixStats *ff_bcast = gCtx->createBroadcastPool("ff", Vector3{1.0f, -4.0f, 1.0f});
    Device::PrefixStats *three_bcast = gCtx->createBroadcastPool("33", Vector3{1.0f, -4.0f, -1.0f});
    Device::PrefixStats *one_bcast = gCtx->createBroadcastPool("01", Vector3{-1.0f, -4.0f, 1.0f});
    Device::PrefixStats *odd_bcast = gCtx->createBroadcastPool("odd", Vector3{-1.0f, -4.0f, -1.0f});

    sCtx->_dst_prefix_group_map.insert(std::make_pair("ff", ff_bcast));
    sCtx->_dst_prefix_group_map.insert(std::make_pair("33", three_bcast));
    sCtx->_dst_prefix_group_map.insert(std::make_pair("01", one_bcast));
    sCtx->_dst_prefix_group_map.insert(std::make_pair("odd", odd_bcast));


    for (int i = 0; i < 58; i++) {
	//std::string *mac_src = new std::string(17, ' ');
	//gen_random(mac_src, 2);
	std::string *mac_src = new std::string("0e:6f:66:05:19:" + std::to_string(i));

        Device::Stats *d_s = gCtx->createSphere(sCtx, *mac_src);
        //sCtx->_device_map.insert(std::make_pair(*mac_src, d_s));
        gCtx->addDirectLabels(d_s);
	gCtx->createIPv4Address(sCtx, "1.0.1.1", d_s->circPoint);
   }
}


void Application::drawTextElements() {
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha, GL::Renderer::BlendFunction::OneMinusSourceAlpha);
    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add, GL::Renderer::BlendEquation::Add);

    gCtx->_text_shader.bindVectorTexture(gCtx->_glyphCache.texture());

    gCtx->_camera->draw(gCtx->_text_drawables);

    GL::Renderer::disable(GL::Renderer::Feature::Blending);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::Zero);
    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add, GL::Renderer::BlendEquation::Add);

    gCtx->_camera->draw(gCtx->_billboard_drawables);
}




void Application::drawIMGuiElements() {
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

    /*
    if (!brokerCtx->peer_connected && _iface_list.size() > 0) {
        if (ImGui::Button("Connect", ImVec2(80, 20))) {
            // If an invalid iface is selected, or an empty value is used
            // just use the first interface.
            auto search = find(_iface_list.begin(), _iface_list.end(), _chosen_iface);
            if (_chosen_iface.length() == 0 ||
                search == _iface_list.end()) {
                _chosen_iface = _iface_list.at(0);
            }

            std::string s, cmd;
            if (_listeningDevice == nullptr) {
                s = "monopt_iface_proto mac_addr ";
                cmd = s.append(_chosen_iface);
                std::string mac_addr = Util::exec_output(cmd);

                if (mac_addr.size() > 0) {
                    _listeningDevice = gCtx->createSphere(sCtx, mac_addr);
                    //objectClicked(_listeningDevice);

                    s = "monopt_iface_proto ipv4_addr ";
                    cmd = s.append(_chosen_iface);
                    std::string ipv4_addr = Util::exec_output(cmd);

                    if (ipv4_addr.size() > 0) {
                        //_listeningDevice->updateMaps(ipv4_addr, "");
                    }
                    gCtx->addDirectLabels(_listeningDevice);
                    //createIPv4Address(ipv4_addr, _listeningDevice->circPoint);
                } else {
                    std::cerr << "Empty mac addr for net interface: " << _chosen_iface << std::endl;
                }
            }

            s = "monopt_iface_proto gateway_ipv4_addr ";
            cmd = s.append(_chosen_iface);
            std::string gw_ipv4_addr = Util::exec_output(cmd);

            if (_activeGateway == nullptr && gw_ipv4_addr.size() > 0) {
                s = "monopt_iface_proto gateway_mac_addr ";
                cmd = s.append(_chosen_iface)
                       .append(" ")
                       .append(gw_ipv4_addr);

                std::string gw_mac_addr = Util::exec_output(cmd);
                if (gw_mac_addr.size() > 0) {
                    _activeGateway = gCtx->createSphere(sCtx, gw_mac_addr);
                    //_activeGateway->updateMaps("0.0.0.0/32", "");
                    //_activeGateway->updateMaps(gw_ipv4_addr, "");

                    gCtx->addDirectLabels(_activeGateway);
                    //createIPv4Address(gw_ipv4_addr, _activeGateway->circPoint);
                } else {
                    std::cerr << "Empty mac addr for gateway: " << gw_ipv4_addr << std::endl;
                }
            }

            s = "monopt_iface_proto launch ";
            cmd = s.append(_chosen_iface);
            _zeek_pid = Util::exec_output(cmd);
            std::cout << "Launched subprocess with pid: " << _zeek_pid << std::endl;
            brokerCtx->peer_connected = true;
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
            brokerCtx->peer_connected = false;

            DeleteEverything();
        }
    }
    */

	    /*
    int offset = 100;
    if (brokerCtx->peer_connected) {
        ImGui::SameLine(offset);
        auto green = ImVec4(0,1,0,1);
        ImGui::TextColored(green, _chosen_iface.c_str(), ImVec2(80, 20));
    } else {
        for (auto it = _iface_list.begin(); it != _iface_list.end(); it++) {
            ImGui::SameLine(offset);
            offset += 80;
            const char *lbl = (*it).c_str();
            if (*it == _chosen_iface) {
                if (ImGui::Selectable(lbl, true, 0, ImVec2(60, 15))) {
                    _chosen_iface = "";
                }
            } else {
                    if (ImGui::Selectable(lbl, false, 0, ImVec2(60, 15))) {
                    _chosen_iface = *it;
                    }
            }
        }
    }
    */

    ImGui::Text("App average %.3f ms/frame (%.1f FPS)",
            1000.0/Magnum::Double(ImGui::GetIO().Framerate), Magnum::Double(ImGui::GetIO().Framerate));

    //brokerCtx->StatsGui();

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
       gCtx->_cameraRig->rotateY(0.10_degf);
    }

    ImGui::Text("Observed Addresses");
    ImGui::Separator();
    ImGui::BeginChild("Scrolling");
    int i = 1;
    for (auto it = sCtx->_device_map.begin(); it != sCtx->_device_map.end(); it++) {
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


void Application::deselectObject() {
    if (_selectedObject != nullptr) {
        _selectedObject->deleteHighlight();
        _selectedObject = nullptr;
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
    selection->addHighlight(gCtx->_bbitem_shader, gCtx->_billboard_drawables);
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

        auto *obj = new Object3D{&gCtx->_scene};
        dwm->_lineDrawable = new Figure::WorldScreenLink(*obj, 0xffffff_rgbf,
            gCtx->_link_shader,
            gCtx->_drawables);
    }
}


void Application::drawEvent() {

    //brokerCtx->processNetworkEvents(sCtx, gCtx);

    if (frame_cnt % 60 == 0) {
        _iface_list = Util::get_iface_list();
    }


    // Remove packet_lines that have expired from the queue
    std::set<Figure::PacketLineDrawable *>::iterator it;
    for (it = sCtx->_packet_line_queue.begin(); it != sCtx->_packet_line_queue.end(); ) {
        // Note this is an O(N) operation
        Figure::PacketLineDrawable *pl = *it;
        if (pl->_expired) {
            it = sCtx->_packet_line_queue.erase(it);
            delete pl;
        } else {
            ++it;
        }
    }

    // Remove mcast drawables that have expired
    for (auto it2 = sCtx->_dst_prefix_group_map.begin(); it2 != sCtx->_dst_prefix_group_map.end(); it2++) {
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

    gCtx->draw3DElements();

    drawTextElements();

    drawIMGuiElements();

    frame_cnt ++;
    swapBuffers();
    _timeline.nextFrame();
    redraw();
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

    sCtx->_selectable_objects.clear();
    sCtx->_packet_line_queue.clear();
    sCtx->_device_map.clear();
    sCtx->_prefix_group_map.clear();

    _inspected_device_window_list.clear();

    //_dst_prefix_group_map.clear();

    // re-initialize application state;
    _selectedObject = nullptr;
    _listeningDevice = nullptr;
    _activeGateway = nullptr;

    //brokerCtx->resetCtx();

    //brokerCtx->ifaceLongChartMgr.empty();
    //brokerCtx->ifaceChartMgr.empty();

    ring_level = 0;
    pos_in_ring = 0;

    _orbit_toggle = false;

    // TODO the objects under these values are not destroyed.
    // Reset scene state
    gCtx->_drawables = SceneGraph::DrawableGroup3D{};
    gCtx->_selectable_drawables = SceneGraph::DrawableGroup3D{};
    gCtx->_billboard_drawables = SceneGraph::DrawableGroup3D{};
    gCtx->_text_drawables = SceneGraph::DrawableGroup3D{};

    redraw();
}


void Application::viewportEvent(ViewportEvent& event) {
    auto size = event.framebufferSize();
    const Range2Di newViewport = {{}, size};

    gCtx->destroyGLBuffers();
    gCtx->prepareGLBuffers(newViewport);
    gCtx->_camera->setViewport(size);

    _imgui.relayout(Vector2{event.windowSize()}/event.dpiScaling(),
        event.windowSize(), size);
}


void Application::keyPressEvent(KeyEvent& event) {
    if(_imgui.handleKeyPressEvent(event)) return;

    /* Movement */
    if(event.key() == KeyEvent::Key::Down) {
        gCtx->_cameraObject->rotateX(5.0_degf);
    } else if(event.key() == KeyEvent::Key::Up) {
        gCtx->_cameraObject->rotateX(-5.0_degf);
    } else if(event.key() == KeyEvent::Key::Left) {
        gCtx->_cameraRig->rotateY(-5.0_degf);
    } else if(event.key() == KeyEvent::Key::Right) {
        gCtx->_cameraRig->rotateY(5.0_degf);
    }
}


void Application::keyReleaseEvent(KeyEvent& event) {
    if(_imgui.handleKeyReleaseEvent(event)) return;


    if(event.key() == KeyEvent::Key::M && _selectedObject != nullptr) {
        std::cout << "m released" << std::endl;
        //Vector3 t = _selectedObject->getObj().transformationMatrix().translation();
        // CREATE plan ring centered on object; add object that follows pos.
        Vector3 v = Vector3{0.0f};
        Util::createLayoutRing(_selectedObject->getObj(), gCtx->_drawables, 30.0, v);


        // project pos onto plane surface.
    }
}


void Application::mousePressEvent(MouseEvent& event) {
    if(_imgui.handleMousePressEvent(event)) return;


    if(event.button() == MouseEvent::Button::Left) {
        _previousMousePosition = _mousePressPosition = event.position();
    }

    event.setAccepted();
}


void Application::mouseReleaseEvent(MouseEvent& event) {
    if(_imgui.handleMouseReleaseEvent(event)) return;

    auto btn = event.button();
    if(!(btn == MouseEvent::Button::Left || btn == MouseEvent::Button::Right)) return;


    if(event.button() == MouseEvent::Button::Left) {
    /* Read object ID at given click position (framebuffer has Y up while windowing system Y down) */
    gCtx->_objselect_framebuffer.mapForRead(GL::Framebuffer::ColorAttachment{0});
    Image2D data = gCtx->_objselect_framebuffer.read(
        Range2Di::fromSize({event.position().x(),
        gCtx->_objselect_framebuffer.viewport().sizeY() - event.position().y() - 1},
        {1, 1}),
        {PixelFormat::R32UI});

    deselectObject();
    UnsignedByte id = Containers::arrayCast<UnsignedByte>(data.data())[0];
    unsigned short i = static_cast<unsigned short>(id);
    if(i > 0 && i < sCtx->_selectable_objects.size()+1) {
        Device::Selectable *selection = sCtx->_selectable_objects.at(i-1);

        objectClicked(selection);

        if (btn == MouseEvent::Button::Left) {
            gCtx->_cameraRig->resetTransformation();
            gCtx->_cameraRig->translate(selection->getTranslation());
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

    if(!(event.buttons() & MouseMoveEvent::Button::Left)) return;

    const Vector2 delta = 3.0f*
        Vector2{event.position() - _previousMousePosition}/
        Vector2{GL::defaultFramebuffer.viewport().size()};

    (*gCtx->_cameraObject)
        .rotate(Rad{-delta.y()}, gCtx->_cameraObject->transformation().right().normalized())
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

    auto c = gCtx->_cameraObject->transformationMatrix().translation();
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
    gCtx->_cameraObject->translateLocal(d);

    redraw();
}


void Application::textInputEvent(TextInputEvent& event) {
    if(_imgui.handleTextInputEvent(event)) return;
}

}

MAGNUM_APPLICATION_MAIN(Monopticon::Application)
