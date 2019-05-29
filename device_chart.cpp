#include <stdio.h>
#include <iostream>
#include <math.h>
#include <ctime>
#include <vector>
#include <algorithm>

#include <imgui.h>

#include "evenbettercap.h"

namespace Poliopticon {

DeviceChartMngr::DeviceChartMngr(int len, float f) {
    vec = std::vector<float>(len);
    arr_len = len;
    moving_avg = 0.0f;
    scaling_factor = f;
}

void DeviceChartMngr::draw() {
    char txt[40];
    sprintf(txt, "avg ppb %0.2f", moving_avg);

    int len = vec.size();

    float y_max = scaling_factor*moving_avg;

    float* arr_ptr = &vec[0];

    // TODO split TX and RX
    ImGui::PlotHistogram("", arr_ptr, len, 0, txt, 0.0f, y_max, ImVec2(300,60));
}


void DeviceChartMngr::clear() {

}

void DeviceChartMngr::resize(int len) {
    vec.resize(len);
    arr_len = len;
}

void DeviceChartMngr::push(float new_val) {
    float prev_val = 0.0f;
    if (vec.size() > arr_len) {
        prev_val = vec.at(0);
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


DeviceWindowMngr::DeviceWindowMngr() {
    chartMngr = new DeviceChartMngr(240, 1.5f);
}

void DeviceWindowMngr::draw() {
    ImGui::SetNextWindowSize(ImVec2(315, 200), ImGuiCond_FirstUseEver);
    bool p_open = true;
    if(!ImGui::Begin("dwm test", &p_open)) {
        ImGui::End();
        return;
    }
    ImGui::Text("device inspector");
    if (chartMngr != NULL) {
        chartMngr->draw();
    }
    ImGui::End();
}

}
