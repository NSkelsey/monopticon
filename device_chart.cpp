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
    welford_m2 = 0.0f;
    moving_avg = 0.0f;
    scaling_factor = f;
}

void DeviceChartMngr::draw() {
    char txt[40];
    sprintf(txt, "avg ppb %0.2f", moving_avg);

    int len = vec.size();

   // float y_max = 6*sqrt(welford_m2)*moving_avg+moving_avg;
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

    //float n = static_cast<float>(arr_len);

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

    /*
    if (prev_val != 0.0f) {
       moving_avg -= prev_val/non_zero_v;
       non_zero_v --;
       if (moving_avg < 0.0f) {
           moving_avg = 0.0f;
       }

    }

    if (new_val == 0.0f) {
        return;
    } else {
       non_zero_v ++;
    }


    //float delta = new_val - moving_avg;
    moving_avg = moving_avg + new_val/non_zero_v;

    if (moving_avg < 0.0f) {
        moving_avg = 0.0f;
    }

    *
    //max_val = *std::max_element(vec.begin(), vec.end());

    float delta2 = new_val - moving_avg;
    welford_m2 = (welford_m2 + delta * delta2)/(arr_len-1);

    */
}

}
