TODO
=====
As of 27/5/19

[X] Cleanup & Docs:
    - Readme with installation instructions
    - Remove wireshark deps and migrate to singular CMake

[ ] Improve source of Evenbettercap
    - Split files from single source file
    - Add version

[ ] OpenGL shader:
    - animate movement to point B for packetline shader
    - remove fade to black (should be fade to white)

    [ ] Selection of objects
        - Increase area of object with highlight or bounding box

[ ] Improve layout:
    - demo circle layout that expands
    - maybe implement  a Radial Balloon Tree with groups based on MACs (bcast, gway, trusted, unknown)

[ ] Improve interface:
    - add GUI display of devices

[ ] Improve analysis
    - add colors for ARP req/resp
        - figure out how to filter to deduplicate arp from raw packets
    - parse dhcp to intuit network topology

[ ] Test Portability // Document Deps
    - Try to run program on Riccardo's machine.

[ ] Improve Scaling
    - Dies at 65355 "too large scene" Corrade assert

[ ] Expirements to describe:
    - [ ] Timeline (with sliding window of pcap)
    - [ ] Port Manifold Shader and Setup
