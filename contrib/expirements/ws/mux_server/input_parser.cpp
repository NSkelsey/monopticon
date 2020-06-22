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
        uint64_t(a[5]) << 40 |
        uint64_t(a[4]) << 32 | (
            // 32-bit instructions take fewer bytes on x86, so use them as much as possible.
            uint32_t(a[3]) << 24 |
            uint32_t(a[2]) << 16 |
            uint32_t(a[1]) << 8 |
            uint32_t(a[0])
        );
}

uint32_t addr_to_ip(caf::ipv6_address::array_type addr) {
    uint32_t ip = (
    addr.at(15) << 24 |
    addr.at(14) << 16 |
    addr.at(13) << 8 |
    addr.at(12)
    );
    return ip;
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

        int parse_l2_summary(epoch::L2Summary* es_l2sum, broker::vector* l2summary);
        void parse_single_mcast(epoch::DeviceComm* dev_comm, int pos, std::string v, broker::vector *dComm);
        void parse_bcast_summaries(epoch::DeviceComm* dev_comm, broker::vector *dComm);
        void parse_enter_l3_addr(epoch::EpochStep* step, std::map<broker::data, broker::data> *addr_map);
        void parse_arp_table(epoch::EpochStep* step, std::map<broker::data, broker::data> *arp_table);

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

            int sum = parse_l2_summary(tx_summary, l2summary);

            pkt_tot += sum;
        }

        parse_bcast_summaries(dev_comm, dComm);
    }

    std::map<broker::data, broker::data> *enter_l2_ipv4_addr_src = broker::get_if<broker::table>(wrapper->at(2));
    if (enter_l2_ipv4_addr_src == nullptr) {
        std::cerr << "l2_ipv4_addr_src" << std::endl;
        return step;
    }

    parse_enter_l3_addr(&step, enter_l2_ipv4_addr_src);

    std::map<broker::data, broker::data> *enter_arp_table = broker::get_if<broker::table>(wrapper->at(3));
    if (enter_l2_ipv4_addr_src == nullptr) {
        std::cerr << "enter_arp_table" << std::endl;
        return step;
    }
    parse_arp_table(&step, enter_arp_table);

    return step;
}


void BrokerCtx::parse_single_mcast(epoch::DeviceComm* dev_comm, int pos, std::string v, broker::vector *dComm) {
    auto *bcast_val = broker::get_if<broker::vector>(dComm->at(pos));
    if (bcast_val == nullptr) {
        return;
    }

    // TODO use Utils::enum
    if (v == "ff") {
        parse_l2_summary(dev_comm->mutable_bcast_ff(), bcast_val);
    } else if (v == "33") {
        parse_l2_summary(dev_comm->mutable_bcast_33(), bcast_val);
    } else if (v == "01") {
        parse_l2_summary(dev_comm->mutable_bcast_01(), bcast_val);
    } else if (v == "odd") {
        parse_l2_summary(dev_comm->mutable_bcast_xx(), bcast_val);
    } else {}
    return;
}

void BrokerCtx::parse_bcast_summaries(epoch::DeviceComm* dev_comm, broker::vector *dComm) {
   parse_single_mcast(dev_comm, 2, "ff", dComm);
   parse_single_mcast(dev_comm, 3, "33", dComm);
   parse_single_mcast(dev_comm, 4, "01", dComm);
   parse_single_mcast(dev_comm, 5, "odd", dComm);
}


int BrokerCtx::parse_l2_summary(epoch::L2Summary* es_l2sum, broker::vector* l2summary) {
    int sum = 0;
    auto *ipv4_cnt = broker::get_if<broker::count>(l2summary->at(0));
    if (ipv4_cnt == nullptr) {
        std::cerr << "ipv4_cnt" << std::endl;
        return 0;
    }
    es_l2sum->set_ipv4(*ipv4_cnt);

    auto *ipv6_cnt = broker::get_if<broker::count>(l2summary->at(1));
    if (ipv6_cnt == nullptr) {
        std::cerr << "ipv6_cnt" << std::endl;
        return 0;
    }
    es_l2sum->set_ipv6(*ipv6_cnt);

    auto *arp_cnt = broker::get_if<broker::count>(l2summary->at(2));
    if (arp_cnt == nullptr) {
        std::cerr << "arp_cnt" << std::endl;
        return 0;
    }
    es_l2sum->set_arp(*arp_cnt);

    auto *unknown_cnt = broker::get_if<broker::count>(l2summary->at(3));
    if (unknown_cnt == nullptr) {
        std::cerr << "unknown_cnt" << std::endl;
        return 0;
    }
    es_l2sum->set_unknown(*unknown_cnt);

    sum = *ipv4_cnt + *ipv6_cnt + *arp_cnt + *unknown_cnt;
    return sum;
}


void BrokerCtx::parse_enter_l3_addr(epoch::EpochStep *step, std::map<broker::data, broker::data> *addr_map) {
     for (auto it = addr_map->begin(); it != addr_map->end(); it++) {
        auto pair = *it;
        auto *mac_src = broker::get_if<std::string>(pair.first);
        if (mac_src == nullptr) {
            std::cerr << "mac_src l2_ipv4_addr:" << std::endl;
            continue;
        }

        epoch::AddrAssoc *addr_assoc = step->add_enter_l2_ipv4_addr_src();
        addr_assoc->set_mac_src(string_to_mac(*mac_src));

        auto *ip_addr_src = broker::get_if<broker::address>(pair.second);
        if (ip_addr_src == nullptr) {
            std::cerr << "ip_addr_src l2_ipv4_addr:" << *mac_src << std::endl;
            continue;
        }

        uint32_t ipv4 = addr_to_ip(ip_addr_src->bytes());
        addr_assoc->set_ipv4(ipv4);
    }
}


void BrokerCtx::parse_arp_table(epoch::EpochStep *step, std::map<broker::data, broker::data> *arp_table) {
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

        epoch::ArpAssoc *arp_assoc = step->add_enter_arp_table();
        arp_assoc->set_mac_src(string_to_mac(*mac_src));

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

            epoch::AddrAssoc *addr_assoc = arp_assoc->add_table_row();
            addr_assoc->set_ipv4(addr_to_ip(ip_addr_dst->bytes()));
            addr_assoc->set_mac_src(string_to_mac(*mac_dst));
        }
    }
}

#endif