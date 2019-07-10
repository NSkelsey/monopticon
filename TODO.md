TODO
=====
As of 10/7/19

- [ ] Add functionality to signalling logic
    - [ ] support prefix pools
    - [ ] support broadcast pools
    - [ ] add ip information to nodes
    - [ ] maybe expire devices after timeout with `exit`

- [ ] Improve Navigation
    - [ ] Zooming
    - [ ] Move camera to orbit other points

- [ ] Improve layout:
    - [x] demo circle layout that expands
        - [ ] Add list of known positions for known devices
    - maybe implement a Radial Balloon Tree with groups based on MACs (bcast, gway, trusted, unknown)

- [ ] Add Strict Ordering:
    - [x] figure out how to generate raw packet events from pcaps
        - [ Use tcpreplay to resend packets on some net interface ]
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
    - [ ] Add sounds for specific events
    - [ ] Create a flat band mesh instead of single pix width wireframes

DONE
====
- [x] Structural/design for sampling at intervals:
    - Refactor to sending epoch messages at 240Hz (TBD) where:
        - Every message contains all of the information neccessary to visualizeall captured events that occured within that interval.
        - The draw loop processes some number of epoch steps every frame: adding, moving and removing objects in the scene
        - The amount of state stored within the main process is minimized in favor of management within zeek.

    - Tasks:
        - [x] Test scheduling and intervals with zeek at sub second frequencies
        - [x] Test windowing and SumStats to track state across windows with zeek
        - [x] List all possible information collected for types sent to monopticon
            - new device
            - new router
            - packet `(l2type, mac_src, mac_dst, ip_src, ip_dst, size)`
            - (api request) inferred ARP tables
        - [x] Refactor `processNetEvents()` to handle messaging format
            - [x] empircally measure arrival frequency
    Note: For large simple traffic flows between two hosts this optimization handles the load well.
          Instead for large amounts of traffic flow coming to and from large numbers of devices, the
          app still experiences large fps drops. Further investigation is needed as well as a good
          measurement framework
    Soln: Disabling --debug inside of the zeek-broker and switching to sampling when event processing
           speeds drop keeps performance around 18ms at 4500 packets per second (nmap -T5 /24 + noise)

- [x] Improve interface:
    - [x] Fix device select menu
        -x change to buttons
        -x remove packet counts
    - [x] add GUI display for devices
        - [x] selected device displays detailed info if we have it.
        - [x] starts a graph of traffic using CharMngr

- [x] Implement watched item highlights
    - [x] semi-opaque line that connects imgui window with watched device
        - [x] Map screen space coords to world space for the vertex shader
    - [x] device window shows all known info

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
