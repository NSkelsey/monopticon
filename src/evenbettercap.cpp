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

using namespace Magnum;
using namespace Math::Literals;

std::string ws_uri = "ws://localhost:9002/";

class Application: public Platform::Application {
    public:
        explicit Application(const Arguments& arguments);

        void prepareDrawables();
        void createLayout(std::string choice);

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

        Context::Graphic  *gCtx;
        Context::Store    *sCtx;
        Context::WsBroker *wCtx;

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
        bool _draggedMouse{false};
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

    gCtx = new Context::Graphic();
    sCtx = new Context::Store();
    wCtx = new Context::WsBroker(ws_uri, gCtx, sCtx);

    srand(time(nullptr));

    _imgui = ImGuiIntegration::Context(Vector2{windowSize()}/dpiScaling(),
        windowSize(), framebufferSize());

    run_sum = 0;
    frame_cnt = 0;

    //setSwapInterval(1);
    _timeline.start();

    //_iface_list = Util::get_iface_list();
    _zeek_pid = "#nop";

    prepareDrawables();

    // TODO add imgui element to choose and for enable/disable of this interface.
    createLayout("cyberlab");
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
}


const char* node_types[] =
{
    "null", "document", "element", "pcdata", "cdata", "comment", "pi", "declaration"
};

struct simple_walker: pugi::xml_tree_walker
{
    std::map<std::string, std::vector<pugi::xml_node*>> object_hierarchy;


    void save(std::string key, pugi::xml_node *n) {

        auto tup = object_hierarchy.find(key);
        if (tup != object_hierarchy.end()) {
            std::vector<pugi::xml_node*> list = tup->second;
            list.push_back(n);
        } else {
            object_hierarchy.emplace(key, std::vector<pugi::xml_node*>{n});
        }

        // TODO convert to some other object here.
    }

    virtual bool for_each(pugi::xml_node& node)
    {
        for (int i = 0; i < depth(); ++i) std::cout << "  "; // indentation

        std::string name = node.name();
        std::cout << node_types[node.type()] << ": name='" << name << "', value='" << node.value() << "'\n";

        std::string parent = node.attribute("parent").value();

        if (name == "object") {
            std::string val = node.attribute("class").value();

            save(parent, &node);
        }

        if (name == "mxCell") {
            if (parent == "") {
                std::cout << "found root" << std::endl;
            }
            if (parent == "1") {
                save(parent, &node);
            }
        }

        if (name == "mxGeometry") {
            auto parent_node = node.parent();
            // MXcell points to parent object while node.parent().parent() is cur id.

            //convert_geometry(node);
        }
        //node.child().child()

        return true; // continue traversal
    }
};

/*
 * Extracts the x & y position from an XML <object> that contains the nested mxGeometry element.
 */
Vector2 get_object_geom(pugi::xml_node obj_node) {
    std::string x_str = obj_node.child("mxCell").child("mxGeometry").attribute("x").value();
    std::string y_str = obj_node.child("mxCell").child("mxGeometry").attribute("y").value();

    if (x_str.size() == 0 ) {
        x_str = "0";
    }

    if (y_str.size() == 0 ) {
        y_str = "0";
    }

    // TODO NOTE float division! TODO TODO
    Vector2 position = Vector2{stof(x_str)/10.0f, stof(y_str)/10.0f};
    return position;
}


Layout::RouterParam* extractRouterParam(pugi::xpath_node router, pugi::xml_document *doc) {
    Layout::RouterParam* param = new Layout::RouterParam{};

    // Extract root Id
    std::string id = router.node().attribute("id").value();
    std::cout << "Extracted id: " << id << std::endl;

    // Get and set the root position
    Vector2 root_pos = get_object_geom(router.node());
    param->pos = Vector3(root_pos.x(), 0.0, root_pos.y());

    std::string xpath = "//object[@class='switch-rect']/mxCell[@parent='" + id + "']";
    pugi::xpath_node sub_rect = doc->select_node(xpath.c_str());
    if (!sub_rect) {
        std::cerr << "Couldn't find subrect: " << xpath << std::endl;
        return param;
    }
    // Get the label of the router
    pugi::xml_node sub_rect_obj = sub_rect.node().parent();
    param->label = sub_rect_obj.attribute("label").value();

    // Get the sub rectangles geometry
    Vector2 sub_pos = get_object_geom(sub_rect_obj);

    // Generate a selector for devices from the switches id
    xpath = "//object[@class='device']/mxCell[@parent='" + id + "']";

    // Extract each attached device's geometry and label
    pugi::xpath_node_set devices = doc->select_nodes(xpath.c_str());
    for (auto it = devices.begin(); it != devices.end(); it++) {
        pugi::xpath_node d_xpath = *it;

        pugi::xml_node dev_obj = d_xpath.node().parent();
        Vector2 dev_pos = get_object_geom(dev_obj);
        std::string dev_lbl = dev_obj.attribute("label").value();

        Layout::RIface *iface = new Layout::RIface{dev_lbl, dev_pos, dev_lbl, 100};
        param->ifaces.push_back(iface);
    }


    return param;
}


void Application::createLayout(std::string choice) {
    Layout::Scenario scenario = Layout::Scenario(choice);

    if (choice == "cyberlab") {

        pugi::xml_document doc;

        Utility::Resource rs("monopticon");
        std::string fname = "src/assets/cyberlab.xml";
        Containers::ArrayView<const char> content = rs.getRaw(fname);

        pugi::xml_parse_result result = doc.load_buffer(&content.front(), content.size());

        pugi::xpath_node_set routers = doc.select_nodes("//object[@class='switch']");
        for (auto it = routers.begin(); it != routers.end(); it++) {
            pugi::xpath_node swtch = *it;
            std::cout << "swtch x: " << swtch.node().name() << std::endl;
            std::cout << swtch.node().child("mxCell") << std::endl;
            std::cout << swtch.node().child("mxCell").child("mxGeometry").attribute("x").value() << std::endl;
            auto rparam = extractRouterParam(swtch, &doc);
            Layout::Router *fw = gCtx->createRouter(sCtx, rparam);
            scenario.add(fw, 1);
        }

        // Classes:
        // center - where the scene should be centered
        // bcast  - the broadcast pool center for a vlan
        //   device - an ellipse positioned on the switch
        // switch - a <g> that contains a switch rect and devices
        //   device - an ellipse positioned on the switch
        //   switch-rect - the geometry of the switch
        // device - a simple device

        auto world_ifaces = std::vector<Layout::RIface*>{};
        world_ifaces.push_back(new Layout::RIface{"blueisp", Vector2(0.0, 0.0), "", 4});
        world_ifaces.push_back(new Layout::RIface{"wan", Vector2(1.0, 4.0), "", 1});
        world_ifaces.push_back(new Layout::RIface{"red", Vector2(2.0, 2.0), "", 2});
        world_ifaces.push_back(new Layout::RIface{"service", Vector2(2.0, 0.0), "", 3});

        //Layout::Router *fw1 = gCtx->createRouter(sCtx, Vector3(-7.0, 0.0, -7.0), "fwWorld", world_ifaces);

        /*
        auto corp_ifaces = std::vector<Layout::RInput*>{};
        corp_ifaces.push_back(new Layout::RInput{"blueisp", Vector2(0.0, 0.0), "", 4});
        corp_ifaces.push_back(new Layout::RInput{"dmz", Vector2(2.0, 3.0), "", 5});
        corp_ifaces.push_back(new Layout::RInput{"lansrv", Vector2(2.0, 1.0), "", 6});
        corp_ifaces.push_back(new Layout::RInput{"lansoc", Vector2(2.0, -1.0), "", 7});
        corp_ifaces.push_back(new Layout::RInput{"lanws", Vector2(2.0, -3.0), "", 8});

        Layout::Router *fw2 = gCtx->createRouter(sCtx, Vector3(7.0, 0.0, 7.0), "fwCorp", corp_ifaces);
        */

        // TODO TODO TODO
        // TODO fix the positioning TODO use an angle then use the vlan positioner to add elements
        auto scenar_vlans = std::vector<Layout::VInput*>{};
        scenar_vlans.push_back(new Layout::VInput{"blue_isp", Vector2(0.0, 0.0), 4});
        scenar_vlans.push_back(new Layout::VInput{"red", Vector2(6.0, 6.0), 2});
        scenar_vlans.push_back(new Layout::VInput{"service", Vector2(6.0, 0.0), 3});

        scenar_vlans.push_back(new Layout::VInput{"dmz", Vector2(8.0, 12.0), 5});
        scenar_vlans.push_back(new Layout::VInput{"lansrv", Vector2(8.0, 4.0), 6});
        scenar_vlans.push_back(new Layout::VInput{"lansoc", Vector2(8.0, -8.0), 7});
        scenar_vlans.push_back(new Layout::VInput{"lanws", Vector2(8.0, -12.0), 8});

        for (auto it = scenar_vlans.begin(); it != scenar_vlans.end(); it++) {
            // TODO these need to be relative to the router
            Layout::VInput* vinp = *it;
            Vector3 twod = Vector3(vinp->pos.x(), -1.0, vinp->pos.y());
            Util::createLayoutRing(gCtx->_scene, gCtx->_permanent_drawables, 1.0f, twod);
        }

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


    if (!wCtx->socket_connected) {
        if (ImGui::Button("Connect", ImVec2(80, 20))) {
            wCtx->openSocket(ws_uri);
            prepareDrawables();
        }
    } else {
        if (ImGui::Button("Disconnect", ImVec2(80, 20))) {
            wCtx->closeSocket();
            DeleteEverything();
        }
    }

    int offset = 100;
    ImGui::SameLine(offset);
    if (wCtx->socket_connected) {
        auto green = ImVec4(0,1,0,1);
        ImGui::TextColored(green, "localhost", ImVec2(80, 20));
    } else {
        ImGui::Text("localhost", ImVec2(60, 15));
    }

    ImGui::Text("App average %.3f ms/frame (%.1f FPS)",
            1000.0/Magnum::Double(ImGui::GetIO().Framerate), Magnum::Double(ImGui::GetIO().Framerate));

    wCtx->statsGui();

    ImGui::End();

    ImGui::SetNextWindowSize(ImVec2(315, 215), ImGuiCond_Once);
    ImGui::SetNextWindowPos(ImVec2(360, 0), ImGuiCond_Once);
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
    wCtx->frameUpdate();

    sCtx->frameUpdate();

    gCtx->draw3DElements();

    drawTextElements();

    drawIMGuiElements();

    frame_cnt ++;
    swapBuffers();
    _timeline.nextFrame();
    redraw();
}


void Application::DeleteEverything() {
    {
        for (auto it = sCtx->_selectable_objects.begin(); it != sCtx->_selectable_objects.end(); it++) {
            Device::Selectable *obj = *it;
            delete obj;
        }

        // All the Stats ptrs in device_map are included in _selectable_objects
        sCtx->_selectable_objects.clear();
        sCtx->_device_map.clear();
    }
    {
        for (auto it = sCtx->_packet_line_queue.begin(); it != sCtx->_packet_line_queue.end(); it++) {
            Figure::PacketLineDrawable *obj = *it;
            delete obj;
        }

        sCtx->_packet_line_queue.clear();
    }
    {
        for (auto it = sCtx->_dst_prefix_group_map.begin(); it != sCtx->_dst_prefix_group_map.end(); it++) {
            Device::PrefixStats *obj = it->second;
            delete obj;
        }

        sCtx->_dst_prefix_group_map.clear();
    }
    {
        for (auto it = sCtx->_prefix_group_map.begin(); it != sCtx->_prefix_group_map.end(); it++) {
            Device::PrefixStats *obj = it->second;
            delete obj;
        }

        sCtx->_prefix_group_map.clear();
    }

    _inspected_device_window_list.clear();

    // re-initialize application state;
    _selectedObject = nullptr;
    _listeningDevice = nullptr;
    _activeGateway = nullptr;

    _orbit_toggle = false;

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


    if(event.button() == MouseEvent::Button::Left && !_draggedMouse) {
        deselectObject();

        // First scale the position from being relative to window size to being
        // relative to framebuffer size as those two can be different on HiDPI
        // systems
        const Vector2i position = event.position()*Vector2{framebufferSize()}/Vector2{windowSize()};
        const Vector2i fbPosition{position.x(), GL::defaultFramebuffer.viewport().sizeY() - position.y() - 1};

        gCtx->_objselect_framebuffer.mapForRead(GL::Framebuffer::ColorAttachment{1});
        Image2D data = gCtx->_objselect_framebuffer.read(
            Range2Di::fromSize(fbPosition, {1, 1}),
            {PixelFormat::R32UI});

        UnsignedInt id = data.pixels<UnsignedInt>()[0][0];

        if(id > 0 && id < sCtx->_selectable_objects.size()+1) {
            Device::Selectable *selection = sCtx->_selectable_objects.at(id-1);

            objectClicked(selection);

            gCtx->_cameraRig->resetTransformation();
            gCtx->_cameraRig->translate(selection->getTranslation());
        }
    }

    _draggedMouse = false;
    event.setAccepted();
    redraw();
}


void Application::mouseMoveEvent(MouseMoveEvent& event) {
    if(_imgui.handleMouseMoveEvent(event)) return;

    if(!(event.buttons() & MouseMoveEvent::Button::Left)) return;

    const Vector2 delta = 3.0f*
        Vector2{event.position() - _previousMousePosition}/
        Vector2{GL::defaultFramebuffer.viewport().size()};

    const Deg x_rot = Deg{-delta.x()};
    const Deg y_rot = Deg{-delta.y()};

    // Check if any single rotation was large enough to debounce _draggedMouse
    const Deg threshold = Deg{0.0025};
    if (x_rot > threshold || y_rot > threshold) {
        _draggedMouse = true;
    }

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
