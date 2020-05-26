TODO
=====
### Webopticon

- [ ] Fix object coloring and buffer bindings using latest updates to magnum
- [ ] Build communication tunnel
   - [ ] Decide on Websockets or something else
   - [ ] Implement small demo with simple backend (golang or c++)
   - [ ] Connect simple backend to zeek broker via TBD protocol
- [ ] Necessary MVP tweaks
   - [ ] use vlan tags for layout groups
   - [ ] Move right-click to space
- [ ] Model Cyberlab Network
   - [ ] decide how VPN access to nginx frontend will work.
   - [ ] deploy broker + sbackend + nginx frontend


### Backlog
========================================

### Release v0.4.0;

- [ ] Make interactive.
    - [ ] Select and move.
    - [ ] Group select with
    - [ ] Add action drop down menu for IP cubes
        - with ping
        - [x] watch command

- [ ] Add functionality to signaling logic
    - [x] support prefix pools
        -[x] bogons & selected prefixes transmit just like devices
    - [?] support broadcast pools
        - [ ] add labels
    - [ ] exit L2Devices that have not communicated in 5 mins
        - check once every second in epoch second
        - requires coordination of the device_map w/ zeek and monopt processes


#### Conn-event framework
- [ ] Arbitrary zeek: conn based event framework.
    - Components:
        - stream event to graphics process
        - imgui window
        - this class:
```c++
        class EventWatcher {
            string name // doubles as channel name
            Vec3 color
            Vector <Events> {
                A, B // l3 device references
                new_drawables // line; bbdrawable
                int t // parameter
            }
        }
```

    - (zeek) side:
        (on startup) - to start use programmed list
            - load needed plugins
            - (open - listening channel)
        (on signal) write into brokered `monopt/event_name`:

    - custom zeek-script
        - `redef EventVizList += { "custom_event_name" };
        - `EventMap::EventTable["custom_event_name"] = custom_event;`

    - (streaming graphic):
        - `map<str, EventW> _active_event_subscriptions`
        - reads from currently watched "event_name":
        - lookup and find L3 addresses:
            - create `Event` add to `EventWatcher.events`
                - add L3 device effect (highlighted red)
                - draw a curve and set t
        - in `draw`
            - decrement t
        - check `EventWatcher` remove `Events` and their drawables

    - (peer_connected):
        - open `monopt/event_subscriber`
        - build "EventWatcher" from communicated list.

    - IMGUI widget:
        - [Pick event >]
            [ x icmp-ping ] [ color ]
            [ o ssh-auth-init ] [ color ]
            [ o dhcp-bind ] [ color ]
        - in `drawImGUI()`
        - create `EventWatcher`; write name into `event_subscriber`


- [ ] Add Strict Ordering:
    - [x] figure out how to generate raw packet events from pcaps
        - Use tcpreplay to resend packets on some net interface
    - [ ] determine how the timeline works
    - [ ] slow down time to step through packet interactions

- [ ] Improve layout:
    - [ ] Add list of known positions for known devices
    - maybe implement a Radial Balloon Tree with groups based on MACs (bcast, gway, trusted, unknown)

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
    - [x] Port Manifold Shader and Setup:
    See `expirements/port-manifold`

- [ ] Improve Portability v2:
    - [ ] Package application in .deb

- [ ] Make more bello:
    - [ ] Traffic graphs should zero out or continue sliding after disconnect
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
```

DONE
====
- [x] Docker container in the docker cloud
   - [x] Q: how to include custom built packages included
   - [x] Connect container to travis build
   - [x] Connect travis build to nskelsey.com
- [x] Improve layout:
    - [x] fix label overlap
    - [x] demo circle layout that expands
    - [x] Demo 3D cube layouts

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
