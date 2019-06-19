#include <stdio.h>
#include <iostream>
#include <math.h>
#include <ctime>
#include <vector>
#include <unistd.h>
#include <sstream>

#include <imgui.h>

#include "evenbettercap.h"


using namespace Monopticon::Device;

Stats::Stats(std::string macAddr, Vector3 pos, Figure::DeviceDrawable *dev):
         mac_addr{macAddr},
         _drawable{dev},
         _highlightedDrawable{nullptr},
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


std::string Stats::create_device_string() {
    std::ostringstream stringStream;
    stringStream << this->mac_addr;
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

void Stats::setSelected(bool selected) {
    _selected = selected;

    if (selected) {
        _drawable->resetTParam();
    }
}

bool Stats::isSelected() {
    return _selected;
}

void Stats::updateMaps(std::string mac_src, std::string ip_src, std::string mac_dst, std::string ip_dst) {

    auto test = [&ip_src](std::string s){ return ip_src==s; };

    if (!std::any_of(_emitted_src_ips.begin(), _emitted_src_ips.end(), test)) {
        _emitted_src_ips.push_back(ip_src);
    }

    //_ip_label->updateText(all_src_ips.str());
}

std::string Stats::makeIpLabel() {
    std::stringstream ss;
    int i = 0;
    for (auto it = _emitted_src_ips.begin(); it != _emitted_src_ips.end(); it++) {
        ss << *it << "\n";
        if (i > 3) {
            ss << "..." << "\n";
            break;
        }
        i++;
    }
    return ss.str();
}

WindowMgr::WindowMgr(Stats *d_s) {
    _stats = d_s;

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
    bool p_open = true;

    if(!ImGui::Begin("Device", &p_open)) {
        ImGui::End();
        return;
    }
    _stats->renderText();

    for (auto it = chartMgrList.begin(); it != chartMgrList.end(); it++) {
        (*it)->draw();
    }

    txChart->draw();
    rxChart->draw();

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

void ChartMgr::clear() {

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
