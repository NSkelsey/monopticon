#include "evenbettercap.h"

using namespace Monopticon::Parse;

BrokerCtx::BrokerCtx(std::string addr, uint16_t port) :
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


void BrokerCtx::processNetworkEvents() {
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
                //epoch_packets_sum += parse_epoch_step(event);
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