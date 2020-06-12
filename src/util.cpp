#include "evenbettercap.h"

using namespace Monopticon;


/*
Util::L2Summary Util::parseL2Summary(broker::vector* l2summary) {
    Util::L2Summary sum{0, 0, 0, 0};

    auto *ipv4_cnt = broker::get_if<broker::count>(l2summary->at(0));
    if (ipv4_cnt == nullptr) {
        std::cerr << "ipv4_cnt" << std::endl;
        return sum;
    }
    sum.ipv4_cnt = *ipv4_cnt;

    auto *ipv6_cnt = broker::get_if<broker::count>(l2summary->at(1));
    if (ipv6_cnt == nullptr) {
        std::cerr << "ipv6_cnt" << std::endl;
        return sum;
    }
    sum.ipv6_cnt = *ipv6_cnt;

    auto *arp_cnt = broker::get_if<broker::count>(l2summary->at(2));
    if (arp_cnt == nullptr) {
        std::cerr << "arp_cnt" << std::endl;
        return sum;
    }
    sum.arp_cnt = *arp_cnt;

    auto *unknown_cnt = broker::get_if<broker::count>(l2summary->at(3));
    if (unknown_cnt == nullptr) {
        std::cerr << "unknown_cnt" << std::endl;
        return sum;
    }
    sum.unknown_cnt = *unknown_cnt;
    return sum;
}
*/


int Util::SumTotal(Util::L2Summary struct_l2) {
    return struct_l2.ipv4_cnt + struct_l2.ipv6_cnt + struct_l2.arp_cnt + struct_l2.unknown_cnt;
}


Vector2 Util::randCirclePoint() {
    float r = rand() / (1e9/(2*Math::Constants<float>::pi()));

    float steps = 16.0f;
    float max = 2*Math::Constants<float>::pi();
    float min = 0.0f;

    float zerone = std::round((r-min)*(steps/(max-min)))/steps;
    float res = zerone*(max-min) + min;

    return Vector2{cos(res), sin(res)};
}


Vector2 Util::paramCirclePoint(int num_elem, int pos) {
    float steps = static_cast<float>(num_elem);
    float posf = static_cast<float>(pos);

    float r = (2*Math::Constants<float>::pi()) * (posf/steps);

    float max = 2*Math::Constants<float>::pi();
    float min = 0.0f;

    float zerone = std::round((r-min)*(steps/(max-min)))/steps;
    float res = zerone*(max-min) + min;

    return Vector2{cos(res), sin(res)};
}


Vector2 Util::randOffset(float z) {
    int x = rand() % 2;
    int y = rand() % 2;
    Vector2 v = Vector2{x ? z : -z, y ? z : -z};
    return v;
}


std::vector<std::string> Util::get_iface_list() {
    auto v = std::vector<std::string>{};
    std::string g = "monopt_iface_proto list_ifaces";
    std::string s = "";
    std::stringstream ss(s);
    std::string t;
    while (std::getline(ss,t,'\n')) {
        v.push_back(t);
    }
    return v;
}


Color3 Util::typeColor(Util::L3Type t) {
    using namespace Util;
    Color3 c;
    switch (t) {
        case L3Type::ARP:
            c = 0xffff00_rgbf;
            break;
        case L3Type::IPV4:
            c = 0x00ffff_rgbf;
            break;
        case L3Type::IPV6:
            c = 0xff00ff_rgbf;
            break;
        default:
            c = 0xff0000_rgbf;
    }
    return c;
}


Figure::RingDrawable* Util::createLayoutRing(Object3D &parent, SceneGraph::DrawableGroup3D &group, float r, Vector3 trans) {
    Object3D *obj = new Object3D{&parent};
    Matrix4 scaling = Matrix4::scaling(Vector3{r});
    obj->transform(scaling);
    obj->rotateX(90.0_degf);
    obj->translate(trans);
    return new Figure::RingDrawable{*obj, 0x00ff00_rgbf, group};
}
