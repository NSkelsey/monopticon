TODO
=====
As of 8/7/19

- [ ] Implement watched item highlights
    - [x] semi-opaque line that connects imgui window with watched device
        - [x] Map screen space coords to world space for the vertex shader
    - [ ] device window shows all known info

- [ ] Improve layout:
    - [x] demo circle layout that expands
        - [ ] Add list of known positions for known devices
    - maybe implement a Radial Balloon Tree with groups based on MACs (bcast, gway, trusted, unknown)

- [ ] Improve interface:
    - [ ] Fix device select menu
        - change to buttons
        - remove packet counts
    - [x] add GUI display for devices
        - [ ] selected device displays detailed info if we have it.
        - [x] starts a graph of traffic using CharMngr

- [ ] Add Strict Ordering:
    - [ ] figure out how to generate raw packet events from pcaps
    - [ ] determine how the timeline works
    - [ ] slow down time to step through packet interactions

- [ ] Improve analysis
    - [ ] add colors for ARP req/resp
        - figure out how to filter to deduplicate arp from raw packets
    - parse dhcp to intuit network topology
    - [x] group broadcast and multicast domains
    - [ ] try to reconstruct arp table of devices inside of the broadcast domain.

- [ ] Add packet filtering
    - Create a status_subscriber to monitor the broker conn: [zeek docs](https://bro-broker.readthedocs.io/en/stable/comm.html#status-and-error-messages)
    - analyze BPF syntax and zeek configuration - exclude filters via zeek module
    - add ui elements (text bar, minimal lookback)
    - apply filters

- [ ] Improve Scaling
    - [x] Dies at 65355 "too large scene" Corrade assert // handled with hardcoded limit for now
    - Produce summary inside of zeek script that remains constant size
    - Use different levels of detail:
        - Every packet
        - Medium (connections)
        - High (summarizes of connections)

- [ ] Improve Logging
    - [x] Broker does not log well (compile time options)
    - [ ] Use log levels inside of monopticon
        - [ ] configurable with a command line flag

- [ ] Expirements to attempt:
    - [ ] Timeline (with sliding window of pcap)
    - [ ] Port Manifold Shader and Setup

- [ ] Improve Portability v2:
    - [ ] Package application in .deb

- [ ] Make more bello:
    - [ ] Add a skybox that simulates __cyberspace__
    - [ ] Add for specific events sounds

DONE
====
- [x] Implement Billboards
    - [x] bounding box for selected objects
    - [x] text label above object
    - [x] Selection of objects
        - Increase area of object with highlight or bounding box

- [x] Temporarily fade out devices after 60 seconds

- [x] OpenGL shader:
    - [x] Fix parametric eqn to window a to b correctly
    - [x] remove fade to black (should be fade to white)

- [x] Tap functionality
    - [x] scripting interface for: start interface, stop interface, check status,
          get routes, get iface, get gateway

- [x] Improve Portability:
    - [x] create and maintain a package for zeek to ease the installation
    - [x] figure out how to force zeek_init instead of the deprecated bro_init
    - [x] internal: ask for a strace of execution
    - [x] plan binary distribution via static build (not possible)
          - [x] ensure that the script file is installed on the system

- [x] Release source
    - Split files from single source file
    - Add version

- [x] Test Portability:
    - Try to run program on Diego's machine:
      communication bug between broker and interface

- [x] Cleanup & Docs:
    - Readme with installation instructions
    - Remove wireshark deps and migrate to singular CMake
