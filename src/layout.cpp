#include "evenbettercap.h"

namespace Monopticon {  namespace Layout {

// Create iface device drawables and position them on a subgrid.
Router::Router(int level, int size, Object3D *root) {
    
    // Grid coordinates
    float x_step = 1.25f;
    float y_step = 1.25f;

    // wire mesh dimensions are: x*round_size/2 + y*2
    //int round_size = size + size % 2;
    root = root;

    // Create a parent and a child rectangle.
}

void Router::plugIface(Device::Stats *d_s, uint32_t vlan) {

    _interfaces.push_back(d_s);
    _attached_vlans.insert(std::make_pair(vlan, d_s));
}

Scenario::Scenario(std::string label) {

}

Scenario& Scenario::add(Router *r, int level) {
    return *this;
}

    
} // Layout
} // Monopticon