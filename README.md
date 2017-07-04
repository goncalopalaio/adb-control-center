# adb-control-center

Proof-of-concept (barely works) interface for common android ADB commands used during development.
Connects to ADB server through sockets.

![screenshot](https://github.com/goncalopalaio/adb-control-center/blob/master/imgs/screenshot.png?raw=true)

OpenGL and keyboard;mouse;window events are handled with:
- GLFW 3.3 - www.glfw.org

User interface is rendered with:
- Nuklear - 1.40.0 - public domain - authored from 2015-2017 by Micha Mettke

Socket connections done with:
- sts_net.h - v0.07 - public domain written 2017 by Sebastian Steinhauer

