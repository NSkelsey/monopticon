#ifndef _INCL_PARSER
#define _INCL_PARSER

#include "broker/broker.hh"
#include "broker/message.hh"
#include "broker/zeek.hh"

#include "epoch.pb.hpp"

// directly from https://stackoverflow.com/a/7326381
uint64_t string_to_mac(std::string const& s) {
    unsigned char a[6];
    int last = -1;
    int rc = sscanf(s.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx%n",
                    a + 0, a + 1, a + 2, a + 3, a + 4, a + 5,
                    &last);
    if(rc != 6 || s.size() != last) {
        std::cerr << "Bad MAC" << std::endl;
        std::cout << s << std::endl;
        return 0;
    }
    return
        uint64_t(a[0]) << 40 |
        uint64_t(a[1]) << 32 | (
            // 32-bit instructions take fewer bytes on x86, so use them as much as possible.
            uint32_t(a[2]) << 24 |
            uint32_t(a[3]) << 16 |
            uint32_t(a[4]) << 8 |
            uint32_t(a[5])
        );
}

class BrokerCtx {
    public:
        std::chrono::duration<int64_t, std::nano> curr_pkt_lag;

        int tot_pkt_drop{0};
        int tot_epoch_drop{0};
        int event_cnt{0};

        int inv_sample_rate{1};
        int epoch_packets_sum{0};

        BrokerCtx() {};

        epoch::EpochStep parse_epoch_step(broker::zeek::Event event);
        void parse_enter_l3_addr(std::map<broker::data, broker::data> *addr_map);
        void parse_arp_table(std::map<broker::data, broker::data> *arp_table);

        void parse_stats_update(broker::zeek::Event event);
};

epoch::EpochStep BrokerCtx::parse_epoch_step(broker::zeek::Event event) {
    epoch::EpochStep step = epoch::EpochStep();

    broker::vector parent_content = event.args();

    broker::vector *wrapper = broker::get_if<broker::vector>(parent_content.at(0));
    if (wrapper == nullptr) {
        std::cerr << "wrapper" << std::endl;
        return step;
    }

    broker::set *enter_l2_devices = broker::get_if<broker::set>(wrapper->at(0));
    if (enter_l2_devices == nullptr) {
        std::cerr << "enter_l2_devices" << std::endl;
        return step;
    }

    for (auto it = enter_l2_devices->begin(); it != enter_l2_devices->end(); it++) {
        auto *mac_src = broker::get_if<std::string>(*it);
        if (mac_src == nullptr) {
            std::cerr << "mac_src e_l2_dev" << std::endl;
            return step;
        }

        step.add_enter_l2devices(string_to_mac(*mac_src));

        // TODO found mac_src
    }

    std::map<broker::data, broker::data> *l2_dev_comm = broker::get_if<broker::table>(wrapper->at(1));
    if (l2_dev_comm == nullptr) {
        std::cerr << "l2_dev_comm" << std::endl;
        return step;
    }

    int pkt_tot = 0;

    for (auto it2 = l2_dev_comm->begin(); it2 != l2_dev_comm->end(); it2++) {
        auto pair = *it2;

        auto *mac_src = broker::get_if<std::string>(pair.first);
        if (mac_src == nullptr) {
            std::cerr << "mac_src e_l2_dev" << std::endl;
            continue;
        }

        // TODO handle l2_dev_comm

        auto *dComm = broker::get_if<broker::vector>(pair.second);
        if (dComm == nullptr) {
            std::cerr << "dComm" <<  std::endl;
            continue;
        }

        epoch::DeviceComm *dev_comm = step.add_l2_dev_comm();
        dev_comm->set_mac_src(string_to_mac(*mac_src));

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

            epoch::L2Summary *tx_summary = dev_comm->add_tx_summary();
            tx_summary->set_mac_dst(string_to_mac(*mac_dst));

            auto *l2summary = broker::get_if<broker::vector>(comm_pair.second);
            if (l2summary == nullptr) {
                std::cerr << "l2summary" << mac_src << std::endl;
                continue;
            }

            // TODO
            tx_summary->set_ipv4(1);

            pkt_tot += 1;
        }
    }

    std::map<broker::data, broker::data> *enter_l2_ipv4_addr_src = broker::get_if<broker::table>(wrapper->at(2));
    if (enter_l2_ipv4_addr_src == nullptr) {
        std::cerr << "l2_ipv4_addr_src" << std::endl;
        return step;
    }
    parse_enter_l3_addr(enter_l2_ipv4_addr_src);

    std::map<broker::data, broker::data> *enter_arp_table = broker::get_if<broker::table>(wrapper->at(3));
    if (enter_l2_ipv4_addr_src == nullptr) {
        std::cerr << "enter_arp_table" << std::endl;
        return step;
    }
    parse_arp_table(enter_arp_table);


    return step;
}


void BrokerCtx::parse_enter_l3_addr(std::map<broker::data, broker::data> *addr_map) {
     for (auto it = addr_map->begin(); it != addr_map->end(); it++) {
        auto pair = *it;
        auto *mac_src = broker::get_if<std::string>(pair.first);
        if (mac_src == nullptr) {
            std::cerr << "mac_src l2_ipv4_addr:" << std::endl;
            continue;
        }

        // TODO add enter l3_addr

        auto *ip_addr_src = broker::get_if<broker::address>(pair.second);
        if (ip_addr_src == nullptr) {
            std::cerr << "ip_addr_src l2_ipv4_addr:" << *mac_src << std::endl;
            continue;
        }

        std::string s = to_string(*ip_addr_src);

        std::cout << "New IP!" << s << std::endl;
    }
}

void BrokerCtx::parse_arp_table(std::map<broker::data, broker::data> *arp_table) {
    for (auto it = arp_table->begin(); it != arp_table->end(); it++) {
        auto pair = *it;
        auto *mac_src = broker::get_if<std::string>(pair.first);
        if (mac_src == nullptr) {
            std::cerr << "mac_src arp_table:" << std::endl;
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

            auto *ip_addr_dst = broker::get_if<broker::address>(pair2.second);
            if (ip_addr_dst == nullptr) {
                std::cerr << "ip_addr_dst arp_table:" << *mac_dst << std::endl;
                continue;
            }

            // TODO handle ip_addr_dst
        }
    }
}

#endif