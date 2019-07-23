TODO
=====
As of 23/7/19

### Release Candidate v0.3.0 -- July 26;
> ========================================
- [ ] Make interactive.
    - Select and move.
    - Group select with

- [ ] Add functionality to signalling logic
    - [x] support prefix pools
        -[x] bogons & selected prefixes transmit just like devices
    - [x] support broadcast pools
    - [ ] exit L2Devices that have not communicated in 5 mins
        - check once every second in epoch second

- [ ] Improve layout:
    - [x] demo circle layout that expands
        - [ ] Add list of known positions for known devices
    - maybe implement a Radial Balloon Tree with groups based on MACs (bcast, gway, trusted, unknown)
    - Demo 3D cube layouts

> ========================================
### Backlog

- [ ] Add Strict Ordering:
    - [x] figure out how to generate raw packet events from pcaps
        - Use tcpreplay to resend packets on some net interface
    - [ ] determine how the timeline works
    - [ ] slow down time to step through packet interactions

- [ ] Improve analysis
    - [ ] Use find routers script to add subnets
    - [ ] add colors for ARP req/resp
        - figure out how to filter to deduplicate arp from raw packets
    - [ ] parse dhcp to intuit network topology
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
    - [ ] Add sounds for specific events for __cyberspace__
    - [ ] Create a flat band mesh instead of single pix width wireframes
    - [ ] Add Oscope like persistence to transmissions for __tron__ **Special FX**
    - [ ] Use a drop down menu to select the statistic to graph in the ifaceMgrCharts

- [ ] Create extensible object manipulation API
    - [ ] Add an API (type for records to be included in epoch event)
      possibily use a generic format like:
```
Create thing:
  [$type, $mac]
Destroy thing:
Update thing:
Do Thing:
  [$effect: enum, $window_content: string, $color: uint, $things: string, $dst: string]
Replace thing
```

DONE
====
- [x] Add IP routes and labels to L2Devices:
    - [x] Use an enter `Thing Record` associated to the L2Device
    - Issue updates for inferred arp table routes to the `Thing Record`:
      the epoch_arp_table updates here are the new additions to the device tables
    - [x] Labels are going to be there own entity inside of the visualization.
      - [x] Level3 - Address Class
      - [x] Placed Above first discovered MAC address with IP.

- [x] Bugfix text renderer assertion failure
    - [x] issue might be garbage the values passed from monopt_proto_iface (just print the output ~ this worked)
    - [no luck] use gef to hook the failure, hard to reach and trigger, could be a non initialized value within object

- [x] Delete everthing on disconnect to reduce confusion.
    - its distracting
    - escalated to gitter
    - escalated to valgrind which finds __no memory leaks__ (only definite ones...)
        - libSDL.so leaks and usr/lib//dri/i965 as well
    - To do again:
```zsh
# Generate suppression output
> valgrind --suppressions=minimal.supp --track-origins=no --leak-check=yes --show-leak-kinds=definite --gen-suppressions=all --log-file=minimalraw.log ./bin/monopticon 2> out-with-suppressions.txt
# parse it
> cat minimalraw.log | ./parse_valgrind.sh > minimal.supp
# Use it too analyze changes or interesting poitns
> valgrind --suppressions=minimal.supp --track-origins=no --leak-check=yes --show-leak-kinds=definite ./bin/monopticon
```

- [x] Improve Navigation
    - [x] Zooming (stashed...)
        - understand why cameraRig transformation produce crazy results: it was a local transform issue
    - [x] Move camera to orbit other points: abrupt, but it works well enough for now.
    - [x] Clamp zoom based on distance from the local rotation point

- [x] Reduce framebuffer copying and handle MSAA properly.

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
