#ifndef _INCL_ebc
#define _INCL_ebc

#include <vector>

namespace Poliopticon {

class DeviceChartMngr {
    public:
        long unsigned int arr_len;
        std::vector<float> vec;

        float moving_avg;
        float max_val;
        float scaling_factor;

        DeviceChartMngr(int len, float f);
        void draw();
        void clear();
        void resize(int len);
        void push(float new_val);
};

class DeviceWindowMngr {
    public:
        DeviceChartMngr *chartMngr;
        std::vector<std::string> announced_ips;

        DeviceWindowMngr();

        void draw();

};

}
#endif
