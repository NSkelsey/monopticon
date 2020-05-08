auralcap
--------

Plays audio based on templated rules from network traffic streams. Out of the box, it plays back normal web traffic.

### Installation & Usage

See run() inside of `netplayback.sh` for examples of how to execute auralcap standalone.

Otherwise use `yay auralcap` then run it with `netplayback <iface>`.

Using `netplayback` requires both `zeek` and `capstats` to be reachable via your `$PATH`.
