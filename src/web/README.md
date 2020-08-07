# web frontend

## Quick Start

Place these files in a folder served by a webserver along with `monopticon.wasm` and `monoption.js`.

Those two files are built during the compilation of the application by [Travis](https://travis-ci.org/github/NSkelsey/monopticon).

The actual steps to compile this web frontend can be found in the file `.travis.yml`. You will need to install docker.

Another approach is to just download the latest release.

## Build for Development

Download the docker container from the docker hub.

Clone the monopticon from github.
```bash
> git clone https://github.com/NSkelsey/monopticon
```


Launch the container mounting the shared folder

```bash
> docker run --rm -it -v `pwd`:/home/build/monopticon -v synnick:monopticon-latest bash
```


From the container’s shell use cmake to create the build files by executing:

```bash
> mkdir build && cd build
> cmake .. \
        -DCMAKE_TOOLCHAIN_FILE=/home/build/monopticon/toolchains/generic/Emscripten-wasm.cmake \
        -DCMAKE_PREFIX_PATH=/usr/lib/emscripten/system \
        -DCMAKE_INSTALL_PREFIX=/home/build/out/bin/ \
        -DFREETYPE_LIBRARY=/home/build/.emscripten_cache/wasm/libfreetype.a \
        -DImGui_INCLUDE_DIR=/opt/imgui
```

Now it’s possible to build the web assembly.

```bash
> cmake --build .
```


Inside of the bin directory there will be the freshly compiled files.

```bash
> ls bin/
monopticon.wasm
monopticon.js
```


## Testing in Development

A simple testing setup for development of the frontend uses the above workflow and simply includes the usage of a local http server.

Just copy the files from src/assets/web to build/bin.

Then serve the frontend via python with:

```bash
> python -m http.server 8000
```

Now visit http://localhost:8000. For connections into the mux_server and zeek launch both of these processes beforehand and the whole pipeline should just work.

## Build for Production

Follow the same instructions as the build for development steps, but if you want some private network’s data included that information must be included statically in the wasm binary. To do this follow the instructions in the next section. 



## Creating Custom Layouts of Networks

The advantage of embedding the structure of an existing network into Monopticon is that it looks better. Using the actual structure of the network improves its usability and readability as devices in the same segment tend to communicate with each-other more.

Unfortunately the process is a bit delicate, so some patience and attention to detail is needed. An example of a layout and the files needed are included by default in the development build. To embed the layout a full build of the software is necessary.

Use the files `src/assets/sample-devs.xml` and `src/assets/sample-layout-drawio.xml` as references. These files are included in the build via the file src/resources.conf and then subsequently referenced in the constructor of `Layout::Scenario`.

When you include a custom network in a production build, you must understand that the data will be embedded in the wasm binary itself. This means that anyone that can access that binary will be able to discover endpoint addresses for the `mux_server` and any MAC, IP addresses and the network structure you include in those files.

To create a layout assign names to the devices and switches you want included. Everything is addressed by its MAC. If you need help with MACs and associated IPs use something like aaaalm.

With names in hand create a `devs.xml` file following the format like the one in `sample-devs.xml`. If you happen to use ansible and a simulated network environment, the output of a playbook run in `variables.yml` can be converted via a yml to xml converter to produce this exact format too.
