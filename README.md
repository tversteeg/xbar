xbar
===

A simple X bar written in C using Xlib.

The `libx11-dev` & `libconfig-dev` libraries are required to build.

Example
===

`xbar -p "xbar.cfg"`

With this as a example config file named `xbar.cfg`:

```cfg
delay = 1;
x = 0;
y = 0;
width = 1280;
height = 16;
background = "#CCCCCC";
foreground = "#000000";

text = ( {
		command = "echo 'xbar'";
		font = "fixed";
		x = 0;
		y = 0;
		align = "left";
	}, {
		command = "date +%T";
		font = "fixed";
		x = 0;
		y = 0;
		width = 1280;
		align = "middle";
	}, {
		command = "date +%x";
		font = "fixed";
		x = 0;
		y = 0;
		width = 1280;
		align = "right";
	}
);
```
Which should create something like this (only bigger):

![Screenshot](http://i.imgur.com/IAFWopF.png)

Setup
===

**Install:**

`git clone https://github.com/tversteeg/xbar && cd xbar && sh setup.sh`

**Uninstall:**

`sudo rm /usr/bin/xbar`
