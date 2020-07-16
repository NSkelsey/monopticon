#include "evenbettercap.h"

namespace Monopticon {  namespace Layout {

// TODO Create iface device drawables and position them on a subgrid.
Router::Router(Object3D *root_obj) {
    root = root_obj;
}

void Router::plugIface(Device::Stats *d_s, uint32_t vlan) {
    _interfaces.push_back(d_s);
    _attached_vlans.insert(std::make_pair(vlan, d_s));
}


Scenario::Scenario(std::string label) {
    std::string doc_path;
    std::string scenario_doc_path;

    if (label == "home-wifi") {
        doc_path = "src/assets/home-wifi.xml";
        scenario_doc_path = "src/assets/home-wifi-devs.xml";
    } else if (label == "cyberlab") {
        doc_path = "src/assets/cyberlab.xml";
        scenario_doc_path = "src/assets/cyberlab-devs.xml";
    } else if (label == "sample") {
        doc_path = "src/assets/sample-layout-drawio.xml";
        scenario_doc_path = "src/assets/sample-cmdb.xml";
    } else{
        std::cerr << "Could not find a relevant scenario" << std::endl;
        doc_path = "src/assets/sample-layout-drawio.xml";
        scenario_doc_path = "src/assets/sample-devs.xml";
    }

    pugi::xml_document doc = get_xml_doc(doc_path);
    pugi::xml_document scenario_doc = get_xml_doc(scenario_doc_path);
}


void Scenario::ParseDocCreateScene(Context::Graphic *gCtx, Context::Store *sCtx) {
    // Find the center of the scenario.
    pugi::xpath_node center = doc.select_node("//object[@class='center']");
    if (!center) {
        std::cerr << "Could not find scenario center!" << std::endl;
        return;
    }

    Vector2 center_pos = get_object_geom(center.node());
    gCtx->center = Vector3(center_pos.x(), 0.0, center_pos.y());

    pugi::xpath_node_set routers = doc.select_nodes("//object[@class='switch']");
    for (auto it = routers.begin(); it != routers.end(); it++) {
        pugi::xpath_node router = *it;
        auto rparam = ExtractRouterParam(router, &doc);
        if (rparam == nullptr) {
            continue;
        }
        AnnotateRouterParam(rparam, &scenario_doc);

        Router *fw = gCtx->createRouter(sCtx, rparam);
    }

    std::string ungrouped_devices = "//root/object[@class='device']/mxCell[@parent='1']";
    pugi::xpath_node_set devices = doc.select_nodes(ungrouped_devices.c_str());
    for (auto it = devices.begin(); it != devices.end(); it++) {
        pugi::xpath_node mxcell = *it;

        // The parent of the mxCell is an object that we can use to get the geometry
        pugi::xml_node device_node = mxcell.node().parent();
        Vector2 pos = get_object_geom(device_node);

        std::string lbl = device_node.attribute("label").value();

        Layout::VlanDevice *vdev = new Layout::VlanDevice{lbl, pos, 0, ""};

        std::string scenario_xpath = "/root/" + lbl;
        pugi::xpath_node xnode = scenario_doc.select_node(scenario_xpath.c_str());
        if (!xnode) {
            std::cerr << "Couldn't find dev: " << scenario_xpath << std::endl;
            continue;
        }

        pugi::xml_node scen_dev_node = xnode.node();
        AnnontateVlanDev(vdev, scen_dev_node);

        gCtx->createDevice(sCtx, vdev);
    }
}


pugi::xml_document get_xml_doc(std::string fname) {
    pugi::xml_document doc;

    Utility::Resource rs("monopticon");
    Containers::ArrayView<const char> content = rs.getRaw(fname);

    pugi::xml_parse_result result = doc.load_buffer(&content.front(), content.size());
    if (!result) {
        std::cerr << "Failed to load xml doc: " << fname << std::endl;
    }

    return doc;
}


Vector2 get_object_geom(pugi::xml_node obj_node) {
    float x = obj_node.child("mxCell").child("mxGeometry").attribute("x").as_float();
    float y = obj_node.child("mxCell").child("mxGeometry").attribute("y").as_float();

    float scale_factor = 20.0f;
    Vector2 position = Vector2{x/scale_factor, y/scale_factor};
    return position;
}


uint32_t get_device_vlan(pugi::xml_node dev_obj) {
    int lbl = dev_obj.attribute("label").as_int();
    return lbl;
}


Layout::RouterParam* ExtractRouterParam(pugi::xpath_node router, pugi::xml_document *doc) {
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
        return nullptr;
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
        uint32_t dev_vlan = get_device_vlan(dev_obj);

        Layout::RIface *iface = new Layout::RIface{"", dev_pos, "", dev_vlan, ""};
        param->vlan_iface_map.emplace(dev_vlan, iface);
    }

    return param;
}


void AnnotateRouterParam(Layout::RouterParam *rparam, pugi::xml_document *scenario_doc) {
    std::string xpath = "/root/" + rparam->label;
    pugi::xpath_node x = scenario_doc->select_node(xpath.c_str());
    if (!x) {
        std::cerr << "Failed to find xpath: " << xpath << std::endl;
        return;
    }

    pugi::xml_node router = x.node();
    rparam->vmid = router.child("vm_id").value();

    for (pugi::xml_node net = router.child("network"); net != nullptr; net = net.next_sibling("network")) {
        std::string mac = net.child("mac").text().as_string();
        std::string ip = net.child("ip").text().as_string();
        int tag = net.child("tag").text().as_int();
        // TODO Skip mgmt iface
        if (tag < 0) {
            continue;
        }
        std::string bridge = net.child("bridge").text().as_string();

        auto search = rparam->vlan_iface_map.find(tag);
        if (search == rparam->vlan_iface_map.end()) {
            std::cerr << "Couldn't find vlan: " << tag << " in: " << xpath << std::endl;
            continue;
        }
        Layout::RIface* iface = search->second;

        iface->label = bridge;
        iface->mac = mac;
        iface->ip_addr = ip;
        iface->vlan = tag;
    }
}


void AnnontateVlanDev(Layout::VlanDevice *vdev, pugi::xml_node named_node) {
    vdev->vmid = named_node.child("vm_id").text().as_string();

    for (auto net = named_node.child("network"); net != nullptr; net.next_sibling("network")) {
        int tag = net.child("tag").text().as_int();

        // Each named node has atleast 1 mgmt interface attached with a tag of -1
        if (tag < 0) {
            continue;
        }

        vdev->tag = tag;
        vdev->mac = net.child("mac").text().as_string();
        vdev->ip_addr = net.child("ip").text().as_string();

        break;
    }
}


} // Layout
} // Monopticon