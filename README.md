# eel6591-project
Class Project for EEL6591: Topic to be decided

## To enable OpenFlow support on Debian/Ubuntu:
Install libraries required by OFSID:
```
sudo apt-get install libxml2-dev libboost-signals-dev libboost-filesystem-dev
```
Build OFSID:
```
.../eel6591-project$ cd openflow
.../eel6591-project/openflow$ ./waf configure
.../eel6591-project/openflow$ ./waf build
.../eel6591-project/openflow$ cd ../ns-3.27
.../eel6591-project/ns-3.27$
```
This should create libopenflow.a for the main ns-3 code; you don't need to copy this anywhere, just specify the location as shown in the next step and ```./waf``` will find it.

Then rerun ```waf configure``` for ns-3 like so:
```
.../eel6591-project/ns-3.27$ ./waf configure --build-profile=debug --enable-examples --enable-tests --with-openflow=../openflow
```
followed by a ```./waf``` or a ```./waf build``` to build.

