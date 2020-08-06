mux_server
==========

The web server is the receiver of the epoch events streamed to it by the monitor server. At a high level, this server aggregates streamed epoch events and mirrors them to connected web clients. On one side nginx serves static web content and forwards websocket connections directly to a ‘mux_process’ that streams epoch events via a websocket to a web browser.

The mux_process is a multithreaded process written in C++. It leverages the threading models provided by the websocketpp library to receive the streamed epoch event’s, deserialize them then reserialize them in a different more compact binary representation for transmission to connected client websocket connections. There are many problems when reusing the zeek’s broker wire format.


## Build & Install

The version of the Zeek Broker must exactly match the one installed on the monitor server. Version 3.1.2 was used for all Monopticon v1.0.0 demos.

Please note that these instructions were not tested and are not complete, but a functioning system was setup on Ubuntu 20.04


```bash
> sudo apt install -y libprotobuf-dev g++ flex bison libboost-all-dev libkqueue-dev libevent-dev openvpn unzip cmake libssl-dev libpcap-dev protobuf-compiler
```


### Build & Install zeek for its broker

Download a release of Zeek - `3.1.2` works.

Configure and install zeek ensuring that shared libaries are installed

```bash
> ./configure --disable-python --disable-broker-tests --disable-auxtools --disable-zeekctl --binary-package
> make
> sudo make install
```

### Build the mux_server

You will probably need to recompile the protobuf sources using the version of protobuf installed on your system.

Clone the 0.8.2 version of the websocketpp project. Copy the contents of the `/mux_server` directory into the examples. Compile the examples. Test to see if program was prepared.

These commands are shown below executed from this directory.

```bash

> git clone https://github.com/nskelsey/websocketpp

> cd websocketpp/examples/mux_server
> protoc --cpp_out epoch.proto

> cd ../.. && mkdir websocketpp/build && cd websocketpp/build

> cmake .. -DBUILD_EXAMPLES=ON -DBROKER_ROOT_DIR=/usr/local/zeek
> cmake --build .
```

# Operation

The `build/bin` should now contain the `mux_server` binary.

```bash
> mux_server <broker-port> <websocket-port>

# Good default choices are:
> mux_server 9999 9002
```

## Running the Nginx frontend.

With nginx you can forward websocket connections to the mux_server encrypting them to this endpoint. To handle TLS just use let's encrypt.

A sample nginx server configuration is included in the file `monopt-site`.
