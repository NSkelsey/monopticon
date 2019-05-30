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

Stats::Stats(std::string macAddr, Vector2 pos, Figure::DeviceDrawable *dev):
         mac_addr{macAddr},
         _drawable{dev},
         circPoint{pos}
{
    num_pkts_sent = 0;
    num_pkts_recv = 0;
}

std::string Stats::create_device_string() {
    std::ostringstream stringStream;
    stringStream << this->mac_addr;
    stringStream << " | ";
    stringStream << this->num_pkts_sent;
    stringStream << " | ";
    stringStream << this->num_pkts_recv;
    std::string c = stringStream.str();
    return c;
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
    moving_avg = sum/static_cast<float>(n);
}


WindowMgr::WindowMgr() {
    //auto chartMgr = new ChartMgr(240, 1.5f);
}

void WindowMgr::draw() {
    ImGui::SetNextWindowSize(ImVec2(315, 200), ImGuiCond_FirstUseEver);
    bool p_open = true;
    if(!ImGui::Begin("dwm test", &p_open)) {
        ImGui::End();
        return;
    }
    ImGui::Text("device inspector");
    /*
    if (chartMgr != NULL) {
        chartMgr->draw();
    }*/
    ImGui::End();
}
