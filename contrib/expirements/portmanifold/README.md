Port Manifold
-------------

Tracking state across devices and ports can be easy. In the image below, every port of a printer connected to my local network has a triangle inside of the application window. Its possible as demonstrated here to visualize in realtime which ports were knocked and which ones responded.


![nmap-scan-response](https://raw.githubusercontent.com/nskelsey/monopticon/master/contrib/screens/printer-syn-ack.gif)

Executing `sizeof(SocketDrawable)` returns `216` which indicates an overhead of around 13.5 MB to visualize every socket of a single device in the worst case.
