#include "evenbettercap.h"

using namespace Monopticon::Parse;

BrokerCtx::BrokerCtx(std::string addr, uint16_t port):
    subscriber(_ep.make_subscriber({"monopt/l2", "monopt/stats"})),
    status_subscriber(_ep.make_status_subscriber(true))
{
    std::cout << "Waiting for broker connection" << std::endl;

    auto res = _ep.listen(addr, port);
    if (res == 0) {
        std::cerr << "Could not listen on: ";
        std::cerr << addr << ":" << port << std::endl;
        std::exit(1);
    } else {
        std::cout << "Endpoint listening on: ";
        std::cout << addr << ":" << port << std::endl;
    }

    curr_pkt_lag = broker::timespan(0);
}


void BrokerCtx::processNetworkEvents(Context::Store *sCtx, Context::Graphic *gCtx) {
    // Process all messages from status_subscriber before doing anything
    if (status_subscriber.available()) {
        auto ss_res = status_subscriber.get();

        auto err = caf::get_if<broker::error>(&ss_res);
        if (err != nullptr) {
            std::cerr << "Broker status error: " << err->code() << ", " << to_string(*err) << std::endl;
        }

        auto st = caf::get_if<broker::status>(&ss_res);
        if (st != nullptr) {
            auto ctx = st->context<broker::endpoint_info>();
            if (ctx != nullptr) {
                std::cerr << "Broker status update regarding "
                          << ctx->network->address
                          << ":" << to_string(*st) << std::endl;
            } else {
               std::cerr << "Broker status update:"
                         << to_string(*st) << std::endl;
            }
        }
    }

    event_cnt = 0;
    int processed_event_cnt = 0;

    epoch_packets_sum = 0;

    // Read and parse packets
    for (auto msg : subscriber.poll()) {
        event_cnt++;
        if (event_cnt % inv_sample_rate == 0) {
            broker::topic topic = broker::get_topic(msg);
            broker::zeek::Event event = broker::get_data(msg);
            std::string name = to_string(topic);
            if (name.compare("monopt/l2") == 0) {
                epoch_packets_sum += parse_epoch_step(sCtx, gCtx, event);
            } else if (name.compare("monopt/stats") == 0) {
                parse_stats_update(event);
            } else {
                std::cerr << "Unhandled Event: " << event.name() << std::endl;
            }
            processed_event_cnt ++;
        } else {
            tot_epoch_drop += 1;
        }
        if (event_cnt % 16 == 0 && inv_sample_rate <= 16) {
            inv_sample_rate = inv_sample_rate * 2;
        }
    }

    // TODO NOTE WARNING dropping events on this side can introduce non existent mac_src
    // devices into the graphic
    if (event_cnt < 8 && inv_sample_rate > 1) {
        inv_sample_rate = inv_sample_rate/2;
    }

    ifaceChartMgr.push(static_cast<float>(epoch_packets_sum));
}


int BrokerCtx::parse_epoch_step(Context::Store *sCtx, Context::Graphic *gCtx, broker::zeek::Event event) {
    broker::vector parent_content = event.args();

    broker::vector *wrapper = broker::get_if<broker::vector>(parent_content.at(0));
    if (wrapper == nullptr) {
        std::cerr << "wrapper" << std::endl;
        return 0;
    }

    broker::set *enter_l2_devices = broker::get_if<broker::set>(wrapper->at(0));
    if (enter_l2_devices == nullptr) {
        std::cerr << "enter_l2_devices" << std::endl;
        return 0;
    }

    for (auto it = enter_l2_devices->begin(); it != enter_l2_devices->end(); it++) {
        auto *mac_src = broker::get_if<std::string>(*it);
        if (mac_src == nullptr) {
            std::cerr << "mac_src e_l2_dev" << std::endl;
            return 0;
        }


        // If the mac_src is not already in the _device_map create a new device.
        // This means that a frame with a never before seen source MAC creates a new Device.
        auto search = sCtx->_device_map.find(*mac_src);
        if (search == sCtx->_device_map.end()) {
            Device::Stats *d_s = gCtx->createSphere(sCtx, *mac_src);
            sCtx->_device_map.insert(std::make_pair(*mac_src, d_s));
            gCtx->addDirectLabels(d_s);
        }
    }

    std::map<broker::data, broker::data> *l2_dev_comm = broker::get_if<broker::table>(wrapper->at(1));
    if (l2_dev_comm == nullptr) {
        std::cerr << "l2_dev_comm" << std::endl;
        return 0;
    }

    int pkt_tot = 0;

    for (auto it2 = l2_dev_comm->begin(); it2 != l2_dev_comm->end(); it2++) {
        auto pair = *it2;

        auto *mac_src = broker::get_if<std::string>(pair.first);
        if (mac_src == nullptr) {
            std::cerr << "mac_src e_l2_dev" << std::endl;
            continue;
        }

        Device::Stats *tran_d_s;
        auto search = sCtx->_device_map.find(*mac_src);
        if (search != sCtx->_device_map.end()) {
            tran_d_s = search->second;
        } else {
            std::cerr << "tran_d_s not found! " << *mac_src << std::endl;
            continue;
        }

        auto *dComm = broker::get_if<broker::vector>(pair.second);
        if (dComm == nullptr) {
            std::cerr << "dComm" <<  std::endl;
            continue;
        }

        parse_bcast_summaries(sCtx, gCtx, dComm, tran_d_s);

        std::map<broker::data, broker::data> *tx_summary = broker::get_if<broker::table>(dComm->at(1));
        if (tx_summary == nullptr) {
            std::cerr << "tx_summary" <<  std::endl;
            continue;
        }

        for (auto it3 = tx_summary->begin(); it3 != tx_summary->end(); it3++) {
            auto comm_pair = *it3;
            auto *mac_dst = broker::get_if<std::string>(comm_pair.first);
            if (mac_dst == nullptr) {
                std::cerr << "mac_dst tx_summary:" << mac_src << std::endl;
                continue;
            }

            Device::Stats *recv_d_s;
            auto search = sCtx->_device_map.find(*mac_dst);
            if (search != sCtx->_device_map.end()) {
                recv_d_s = search->second;
            } else {
                std::cerr << "recv_d_s not found! " << *mac_dst << std::endl;
                continue;
            }

            auto *l2summary = broker::get_if<broker::vector>(comm_pair.second);
            if (l2summary == nullptr) {
                std::cerr << "l2summary" << mac_src << std::endl;
                continue;
            }
            Util::L2Summary struct_l2 = Util::parseL2Summary(l2summary);

            Vector3 p1 = tran_d_s->circPoint;
            Vector3 p2 = recv_d_s->circPoint;

            gCtx->createLines(sCtx, p1, p2, Util::L3Type::IPV4, struct_l2.ipv4_cnt);
            gCtx->createLines(sCtx, p1, p2, Util::L3Type::IPV6, struct_l2.ipv6_cnt);
            gCtx->createLines(sCtx, p1, p2, Util::L3Type::ARP, struct_l2.arp_cnt);
            gCtx->createLines(sCtx, p1, p2, Util::L3Type::UNKNOWN, struct_l2.unknown_cnt);
            int dev_tot = Util::SumTotal(struct_l2);
            tran_d_s->num_pkts_sent += dev_tot;
            recv_d_s->num_pkts_recv += dev_tot;

            pkt_tot += dev_tot;
        }
    }

    std::map<broker::data, broker::data> *enter_l2_ipv4_addr_src = broker::get_if<broker::table>(wrapper->at(2));
    if (enter_l2_ipv4_addr_src == nullptr) {
        std::cerr << "l2_ipv4_addr_src" << std::endl;
        return 0;
    }
    parse_enter_l3_addr(sCtx, gCtx, enter_l2_ipv4_addr_src);

    std::map<broker::data, broker::data> *enter_arp_table = broker::get_if<broker::table>(wrapper->at(3));
    if (enter_l2_ipv4_addr_src == nullptr) {
        std::cerr << "enter_arp_table" << std::endl;
        return 0;
    }
    parse_arp_table(sCtx, gCtx, enter_arp_table);


    return pkt_tot;
}


void BrokerCtx::parse_single_mcast(Context::Store *sCtx, Context::Graphic *gCtx, int pos, std::string v, broker::vector *dComm, Device::Stats* tran_d_s) {
    Util::L2Summary sum;
    Device::PrefixStats *dp_s = nullptr;

    auto *bcast_val = broker::get_if<broker::vector>(dComm->at(pos));
    if (bcast_val != nullptr) {
        sum = Util::parseL2Summary(bcast_val);
        dp_s = sCtx->_dst_prefix_group_map.at(v);
        gCtx->createPoolHits(sCtx, tran_d_s, dp_s, sum);
    }
}


void BrokerCtx::parse_bcast_summaries(Context::Store *sCtx, Context::Graphic *gCtx, broker::vector *dComm, Device::Stats* tran_d_s) {
    parse_single_mcast(sCtx, gCtx, 2, "ff", dComm, tran_d_s);
    parse_single_mcast(sCtx, gCtx, 3, "33", dComm, tran_d_s);
    parse_single_mcast(sCtx, gCtx, 4, "01", dComm, tran_d_s);
    parse_single_mcast(sCtx, gCtx, 5, "odd", dComm, tran_d_s);
}


void BrokerCtx::parse_enter_l3_addr(Context::Store *sCtx, Context::Graphic *gCtx, std::map<broker::data, broker::data> *addr_map) {
     for (auto it = addr_map->begin(); it != addr_map->end(); it++) {
        auto pair = *it;
        auto *mac_src = broker::get_if<std::string>(pair.first);
        if (mac_src == nullptr) {
            std::cerr << "mac_src l2_ipv4_addr:" << std::endl;
            continue;
        }

        Device::Stats *tran_d_s;
        auto search = sCtx->_device_map.find(*mac_src);
        if (search != sCtx->_device_map.end()) {
            tran_d_s = search->second;
        } else {
            std::cerr << "tran_d_s l2_ipv4_addr not found! " << *mac_src << std::endl;
            continue;
        }

        auto *ip_addr_src = broker::get_if<broker::address>(pair.second);
        if (ip_addr_src == nullptr) {
            std::cerr << "ip_addr_src l2_ipv4_addr:" << *mac_src << std::endl;
            continue;
        }

        std::string s = to_string(*ip_addr_src);

        gCtx->createIPv4Address(sCtx, s, tran_d_s->circPoint);
    }
}

void BrokerCtx::parse_arp_table(Context::Store *sCtx, Context::Graphic *gCtx, std::map<broker::data, broker::data> *arp_table) {
    for (auto it = arp_table->begin(); it != arp_table->end(); it++) {
        auto pair = *it;
        auto *mac_src = broker::get_if<std::string>(pair.first);
        if (mac_src == nullptr) {
            std::cerr << "mac_src arp_table:" << std::endl;
            continue;
        }

        Device::Stats *tran_d_s;
        auto search = sCtx->_device_map.find(*mac_src);
        if (search != sCtx->_device_map.end()) {
            tran_d_s = search->second;
        } else {
            std::cerr << "mac_src arp_table not found! " << *mac_src << std::endl;
            continue;
        }

        std::map<broker::data, broker::data> *src_table = broker::get_if<broker::table>(pair.second);
        if (src_table == nullptr) {
            std::cerr << "src_table arp_table:" << *mac_src << std::endl;
            continue;
        }

        for (auto it2 = src_table->begin(); it2 != src_table->end(); it2++) {
            auto pair2 = *it2;
            auto *mac_dst = broker::get_if<std::string>(pair2.first);
            if (mac_dst == nullptr) {
                std::cerr << "mac_dst arp_table:" << std::endl;
                continue;
            }

            Device::Stats *recv_d_s;
            auto search2 = sCtx->_device_map.find(*mac_dst);
            if (search2 != sCtx->_device_map.end()) {
                recv_d_s = search2->second;
            } else {
                std::cerr << "mac_dst arp_table not found! " << *mac_dst << std::endl;
                continue;
            }

            auto *ip_addr_dst = broker::get_if<broker::address>(pair2.second);
            if (ip_addr_dst == nullptr) {
                std::cerr << "ip_addr_dst arp_table:" << *mac_dst << std::endl;
                continue;
            }

            const Vector3 offset{0.0f, 1.0f, 0.0f};
            gCtx->addL2ConnectL3(tran_d_s->circPoint, recv_d_s->circPoint+offset);
            gCtx->addL2ConnectL3(recv_d_s->circPoint+offset, tran_d_s->circPoint);
        }
    }
}

void BrokerCtx::StatsGui() {
    auto s = "Sample rate %.3f SPS event cnt %d";
    if (inv_sample_rate > 1.0) {
        ImGui::TextColored(ImVec4(1,0,0,1), s, 1.0/inv_sample_rate, event_cnt);
    } else {
        ImGui::Text(s, 1.0/inv_sample_rate, event_cnt);
    }

    auto r = "Pkt Lag %.1f ms; Pkt drop: %d; Epoch drop: %d";
    double t = curr_pkt_lag.count()/1000000.0;
    if (curr_pkt_lag > std::chrono::milliseconds(5)) {
        ImGui::TextColored(ImVec4(1,0,0,1), r, t, tot_pkt_drop, tot_epoch_drop);
    } else {
        ImGui::Text(r, t, tot_pkt_drop, tot_epoch_drop);
    }

    ImGui::Separator();
    ifaceChartMgr.draw();
    ImGui::Separator();
    ifaceLongChartMgr.draw();
    ImGui::Separator();
}

void BrokerCtx::parse_stats_update(broker::zeek::Event event) {
    broker::vector parent_content = event.args();

    broker::vector *wrapper = broker::get_if<broker::vector>(parent_content.at(0));
    if (wrapper == nullptr) {
        std::cerr << "stat wrapper" << std::endl;
        return;
    }

    broker::count *mem_usage = broker::get_if<broker::count>(wrapper->at(2));
    if (mem_usage == nullptr) {
        std::cerr << "stat mem" << std::endl;
        return;
    }

    broker::count *pkts_proc = broker::get_if<broker::count>(wrapper->at(3));
    if (pkts_proc == nullptr) {
        std::cerr << "stat pkts_proc" << std::endl;
        return;
    }

    // Update iface packet statistics
    ifaceLongChartMgr.push(static_cast<float>(*pkts_proc));

    broker::count *pkts_drop = broker::get_if<broker::count>(wrapper->at(5));
    if (pkts_drop == nullptr) {
        std::cerr << "stat pkts_drop" << std::endl;
        return;
    }
    tot_pkt_drop += (*pkts_drop);

    broker::timespan *pkt_lag = broker::get_if<broker::timespan>(wrapper->at(7));
    if (pkt_lag == nullptr) {
        std::cerr << "stat pkt_lag" << std::endl;
        return;
    }

    curr_pkt_lag = *pkt_lag;
}
