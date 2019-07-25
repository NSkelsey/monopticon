#include "evenbettercap.h"

using namespace Monopticon::Device;


Selectable::~Selectable() {
    std::cerr << "Virtual destruct called!" << std::endl;
    exit(1);
    deleteHighlight();
}


void Selectable::addHighlight(Shaders::Flat3D& shader, SceneGraph::DrawableGroup3D &group) {
    const Color3 c = 0x00ff00_rgbf;
    const Matrix4 scaling = Matrix4::scaling(Vector3{8.0});

    Object3D *o = new Object3D{&this->getObj()};
    o->transform(scaling);

    _highlight = new Figure::UnitBoardDrawable{*o, shader, group, c};
}


void Selectable::deleteHighlight() {
    if (_highlight != nullptr) {
        delete _highlight;
        _highlight = nullptr;
    }
}


bool Selectable::isSelected() {
    return _highlight != nullptr;
}


Object3D& Stats::getObj() {
    Object3D& t = *_drawable;
    return t;
}


Stats::Stats(std::string macAddr, Vector3 pos, Figure::DeviceDrawable *dev):
         mac_addr{macAddr},
         _drawable{dev},
         _ip_label{nullptr},
         _mac_label{nullptr},
         _windowMgr{nullptr},
         _selected{false},
         _emitted_src_ips{},
         _dst_arp_map{},
         circPoint{pos},
        num_pkts_sent{0},
        num_pkts_recv{0},
        health{60*30}
{}


Vector3 Stats::getTranslation() {
    return circPoint;
}


std::string Stats::create_device_string() {
    std::ostringstream stringStream;
    stringStream << "MAC: ";
    stringStream << this->mac_addr;
    if (_emitted_src_ips.size() > 0) {
        stringStream << " IP: ";
        stringStream << _emitted_src_ips.at(0);
    }
    std::string c = stringStream.str();
    return c;
}


void Stats::renderText() {
    std::string s = create_device_string();
    if (isSelected()) {
        ImGui::TextColored(ImVec4(1,1,0,1), "%s", s.c_str());
    } else {
        ImGui::Text("%s", s.c_str());
    }
}


/*
void Stats::setSelected(bool selected) {
    _selected = selected;

    if (selected) {
        _drawable->resetTParam();
    }
}*/


void Stats::updateMaps(std::string ip_src, std::string mac_dst) {

    auto test = [&ip_src](std::string s){ return ip_src==s; };

    if (!std::any_of(_emitted_src_ips.begin(), _emitted_src_ips.end(), test)) {
        _emitted_src_ips.push_back(ip_src);
    }

    //_ip_label->updateText(all_src_ips.str());
    auto it = _dst_arp_map.find(mac_dst);
    if (it != _dst_arp_map.end()) {
        RouteMgr *r_mgr = new RouteMgr{};
        auto p = std::make_pair(mac_dst, r_mgr);
        _dst_arp_map.insert(p);
    }
}


std::string Stats::makeIpLabel() {
    std::stringstream ss;
    int i = 0;
    for (auto it = _emitted_src_ips.begin(); it != _emitted_src_ips.end(); it++) {
        ss << *it << "\n";
        if (i >= 2) {
            ss << "..." << "\n";
            break;
        }
        i++;
    }
    return ss.str();
}


Stats::~Stats() {
    delete _mac_label;
    delete _drawable;
    delete _ip_label;
    delete _mac_label;

    _dst_arp_map.clear();
    _emitted_src_ips.clear();
}


PrefixStats::PrefixStats(std::string macPrefix, Vector3 pos, Figure::RingDrawable* ring):
    contacts{},
    _prefix{macPrefix},
    _position{pos},
    ring{ring}
{}


WindowMgr::WindowMgr(Stats *d_s):
    _lineDrawable{nullptr}
{
    _stats = d_s;
    _win_open = true;

    chartMgrList = std::vector<ChartMgr*>();

    txChart = new ChartMgr(240, 1.5f);
    rxChart = new ChartMgr(240, 1.5f);

    last_frame_tx = _stats->num_pkts_sent;
    last_frame_rx = _stats->num_pkts_recv;
}


void WindowMgr::draw() {
    // TODO place in chartUpdate
    int cur_frame_tx = _stats->num_pkts_sent;
    int diff = cur_frame_tx - last_frame_tx;

    txChart->push(diff);
    last_frame_tx = cur_frame_tx;

    int cur_frame_rx = _stats->num_pkts_recv;
    diff = cur_frame_rx - last_frame_rx;

    rxChart->push(diff);
    last_frame_rx = cur_frame_rx;
    // TODO TODO TODO

    ImGui::SetNextWindowSize(ImVec2(315, 215), ImGuiCond_Once);

    auto s = std::string("Device ").append(_stats->mac_addr);

    auto flags = ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse;

    ImGui::Begin(s.c_str(), &_win_open, flags);

    _stats->renderText();

    for (auto it = chartMgrList.begin(); it != chartMgrList.end(); it++) {
        (*it)->draw();
    }

    auto a = _stats->circPoint;
    ImVec2 b = ImGui::GetWindowPos();

    auto vp = GL::defaultFramebuffer.viewport();
    auto screen_width = vp.x().size();
    auto screen_height = vp.y().size();
    auto x_midp = 157.5f;
    auto y_midp = 107.5f;

    auto c = Vector2(2.0f*(1.0f/screen_width)*(b.x+x_midp) - 1.0f, 1.0f - 2.0f*(1.0f/screen_height)*(b.y+y_midp));
    _lineDrawable->setCoords(a, c);

    txChart->draw();
    rxChart->draw();

    ImGui::Text("Tot tx pkts: %d Tot rx pkts: %d",
                _stats->num_pkts_sent,
                _stats->num_pkts_recv);

    ImGui::Text("Tot observed source IPs: %lu", _stats->_emitted_src_ips.size());

    ImGui::End();
}


ChartMgr::ChartMgr(int len, float f) {
    vec = std::vector<float>(len);
    arr_len = len;
    moving_avg = 0.0f;
    scaling_factor = f;
}


void ChartMgr::draw() {
    char txt[40];
    sprintf(txt, "avg ppb %0.2f", static_cast<double>(moving_avg));

    int len = vec.size();

    float y_max = scaling_factor*moving_avg;

    float* arr_ptr = &vec[0];

    // TODO split TX and RX
    ImGui::PlotHistogram("", arr_ptr, len, 0, txt, 0.0f, y_max, ImVec2(300,60));
}


void ChartMgr::resize(int len) {
    vec.resize(len);
    arr_len = len;
}


void ChartMgr::push(float new_val) {
    if (vec.size() > arr_len) {
        vec.erase(vec.begin());
    }
    vec.push_back(new_val);

    float sum = 0;
    float n = 0;
    for (auto it = vec.begin(); it != vec.end(); it++) {
        auto v = *(it);
        if (v > 0.0f) {
            sum += v;
            n+=1;
        }
    }
    if (n != 0) {
        moving_avg = sum/static_cast<float>(n);
    } else {
        moving_avg = 0.0f;
    }
}
