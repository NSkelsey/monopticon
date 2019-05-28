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
        float welford_m2;
        int non_zero_v;
        float scaling_factor;

        DeviceChartMngr(int len, float f);
        void draw();
        void clear();
        void resize(int len);
        void push(float new_val);
};

/*{
  public:
     DeviceStats(std::string macAddr, Vector2 pos, DeviceDrawable *dev);

     std::string create_device_string();

     std::string mac_addr;
     Vector2 circPoint;
     int num_pkts_sent;
     int num_pkts_recv;
     DeviceDrawable *_drawable;
};*/
}
#endif
