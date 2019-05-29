TODO
=====
As of 29/5/19

- [ ] Improve source of Evenbettercap
    - Split files from single source file
    - Add version

- [ ] OpenGL shader:
    - animate movement to point B for packetline shader
    - remove fade to black (should be fade to white)

    - [ ] Selection of objects
        - Increase area of object with highlight or bounding box

- [ ] Improve layout:
    - demo circle layout that expands
    - maybe implement  a Radial Balloon Tree with groups based on MACs (bcast, gway, trusted, unknown)

- [ ] Improve interface:
    - add GUI display for devices
        - selected device displays detailed info if we have it.
        - starts a graph of traffic using CharMngr

- [ ] Improve analysis
    - add colors for ARP req/resp
        - figure out how to filter to deduplicate arp from raw packets
    - parse dhcp to intuit network topology
    - Try to reconstruct arp table of devices inside of the broadcast domain.

- [ ] Improve Scaling
    - Dies at 65355 "too large scene" Corrade assert // handled with hardcoded limit for now
    - Produce summary inside of zeek script that remains constant size
    - Use different levels of detail:
        - Every packet
        - Medium (connections)
        - High (summarizes of connections)

- [ ] Improve Logging
    - [ ] Broker does not log well

- [ ] Improve Portability:
    - [ ] create and maintain a package for zeek to ease the installation
    - [ ] figure out how to force zeek_init instead of the deprecated bro_init

- [ ] Expirements to attempt:
    - [ ] Timeline (with sliding window of pcap)
    - [ ] Port Manifold Shader and Setup

DONE
====

- [x] Test Portability:
    - Try to run program on Diego's machine:
      communication bug between broker and interface

- [x] Cleanup & Docs:
    - Readme with installation instructions
    - Remove wireshark deps and migrate to singular CMake
