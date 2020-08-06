monitor_server
==============
As described in the design document, the monitor server is a minimal zeek deployment with a custom script. This means zeek, the zeek script and the bash script `monopt_iface_proto` must be prepared and installed.

## Build and Instal

### Setup Zeek

Install and build zeek with shared libraries. Version 3.1.2 is a known functional choice. Use the same installation as described in the `mux_server` installation guide.

### Install the scripts

Use the `CMakeLists.txt` file here to place the files on your filesystem. Do that with:

```bash
> mkdir build && cd build
> cmake ..
> sudo make install
```

## Usage

Ensure that the mux_server is running.

Execute:

> monopt_iface_proto launch <iface> <host:broker-port>
