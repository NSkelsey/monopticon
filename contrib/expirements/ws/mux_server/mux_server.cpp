#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

// This header pulls in the WebSocket++ abstracted thread support that will
// select between boost::thread and std::thread based on how the build system
// is configured.
#include <websocketpp/common/thread.hpp>

#include <websocketpp/config/asio_no_tls.hpp>

#include <websocketpp/server.hpp>

#include <fstream>
#include <iostream>
#include <set>
#include <streambuf>
#include <string>

#include "broker/broker.hh"
#include "broker/message.hh"
#include "broker/zeek.hh"

#include "epoch.pb.hpp"

class BrokerCtx {
public:
    BrokerCtx() {};
    epoch::EpochStep parse_epoch_step(broker::zeek::Event event);
};


class mux_server {
public:
    typedef websocketpp::connection_hdl connection_hdl;

    typedef websocketpp::server<websocketpp::config::asio> server;
    typedef websocketpp::lib::lock_guard<websocketpp::lib::mutex> scoped_lock;

    mux_server() : m_count(0) {
        // set up access channels to only log interesting things
        m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
        m_endpoint.set_access_channels(websocketpp::log::alevel::access_core);
        m_endpoint.set_access_channels(websocketpp::log::alevel::app);

        // Initialize the Asio transport policy
        m_endpoint.init_asio();

        // Bind the handlers we are using
        using websocketpp::lib::placeholders::_1;
        using websocketpp::lib::bind;
        m_endpoint.set_open_handler(bind(&mux_server::on_open,this,_1));
        m_endpoint.set_close_handler(bind(&mux_server::on_close,this,_1));
    }

    void run(const uint16_t port) {
        m_port = port;

        // Create a thread to run the ASIO io_service event loop
        websocketpp::lib::thread asio_thread(&mux_server::serve_sockets, this);

        // Create a thread to run the telemetry loop
        websocketpp::lib::thread telemetry_thread(&mux_server::telemetry_loop,this);

        asio_thread.join();
        telemetry_thread.join();
    }

    void serve_sockets() {
        // listen on specified port
        m_endpoint.listen(m_port);

        // Start the server accept loop
        m_endpoint.start_accept();
        try {
            m_endpoint.run();
        } catch (websocketpp::exception const & e) {
            std::cout << e.what() << std::endl;
        }
    }


    void on_open(connection_hdl hdl) {
        m_connections.insert(hdl);
    }

    void on_close(connection_hdl hdl) {
        m_connections.erase(hdl);
    }

    void telemetry_loop() {
        uint64_t count = 0;
        std::stringstream val;

        std::string addr = "127.0.0.1";
        uint16_t port = 9999;
        BrokerCtx bCtx = BrokerCtx();

        // Zeek broker components
        broker::endpoint _ep;

        broker::subscriber subscriber = _ep.make_subscriber({"monopt/l2", "monopt/stats"});
        broker::status_subscriber status_subscriber = _ep.make_status_subscriber(true);

        int res = _ep.listen(addr, port);

        if (res == 0) {
          std::cerr << "Could not listen on: ";
          std::cerr << addr << ":" << port << std::endl;
          std::exit(1);
        } else {
          std::cout << "Endpoint listening on: ";
          std::cout << addr << ":" << port << std::endl;
        }

        int len = 1024*10;
        char buf[len] = {0};

        int stats_cnt = 0;
        while(1) {
            for (auto msg : subscriber.poll()) {
                broker::topic topic = broker::get_topic(msg);
                broker::zeek::Event event = broker::get_data(msg);
                std::string name = to_string(topic);
                if (name.compare("monopt/l2") == 0) {
                    epoch::EpochStep step = bCtx.parse_epoch_step(event);

                    val.str("");
                    val << "Topic: " << name << " cnt:" << count++;

                    step.SerializePartialToArray(&buf, len);

                    con_list::iterator it;
                    for (it = m_connections.begin(); it != m_connections.end(); ++it) {
                        m_endpoint.send(*it, buf, step.ByteSize(), websocketpp::frame::opcode::binary);
                    }
                } else if (name.compare("monopt/stats") == 0) {
                    //parse_stats_update(event);
                    stats_cnt ++;
                    if (stats_cnt % 50 == 0) {
                        std::cerr << "Saw 50 stats update: " << event.name() << std::endl;
                    }
                } else {
                    std::cerr << "Unhandled Event: " << event.name() << std::endl;
                }

            }
        }
    }

private:
    server m_endpoint;
    websocketpp::lib::mutex m_lock;
    bool m_open;
    bool m_done;

    typedef std::set<connection_hdl,std::owner_less<connection_hdl>> con_list;

    con_list m_connections;
    server::timer_ptr m_timer;

    // Telemetry data
    uint64_t m_count;
    uint16_t m_port;
};

int main(int argc, char* argv[]) {
    mux_server s;

    std::string docroot;
    uint16_t port = 9002;

    if (argc == 1) {
        std::cout << "Usage: mux_server [port]" << std::endl;
        return 1;
    }

    if (argc >= 2) {
        int i = atoi(argv[1]);
        if (i <= 0 || i > 65535) {
            std::cout << "invalid port" << std::endl;
            return 1;
        }

        port = uint16_t(i);
    }

    s.run(port);
    return 0;
}