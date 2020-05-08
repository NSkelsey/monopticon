auralcap
--------

Plays audio based on templated rules from network traffic streams. Out of the box, it plays back normal web traffic.

### Installation

If you just want to check out quickly on Arch run:

```
> yay auralcap
```

### Usage

After this you will have a minimal interface for then monitoring writes to the `event-dir`. 
Anytime a write is flushed to a file in `event-dir`, normally an append, an event is fired
which causes a note to play.

Create a directory to monitor and write to it anytime something you want to hear happens.
For example when something happens on port 22.

```bash
mkdir tmp
tcpdump -n 'port 22' | grep --line-buffered 'IP' >> tmp/ssh-event.log
```

In another terminal run:

```bash
auralcap -N tmp
```

For more advanced usage try `netplayback`

The program requires both `zeek` and `capstats` - a binary included in the zeek package - are reachable via the `$PATH` environment variable.