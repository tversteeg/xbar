xbar
===

A simple X bar written in C using Xlib.

The `libx11-dev` & `libconfig-dev` libraries are required to build.

Example:

`xbar -e "echo \"xbar - \$(date +%T)\""`

To install:

`git clone https://github.com/tversteeg/xbar && cd xbar && sh setup.sh`

To uninstall:

`sudo rm /usr/bin/xbar`
