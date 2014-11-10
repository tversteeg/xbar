xbar
===

A simple X bar written in C using Xlib.

The `libx11-dev` & `libconfig-dev` libraries are required to build.

Example
===

`xbar -p "xbar.cfg"`

With this as a example config file named `xbar.cfg`:

```cfg
location = (0, 0);
size = (1280, 16);
delay = 1;
font = "-*-terminus-*-r-*-*-12-*-*-*-*-*-*-*";

colors = {
	background = "#CCCCCC";
	text = "#000000";
};

text = ( {
		command = "echo 'xbar'";
		font = "fixed";
		location = (0, 0);
		width = 1280;
		align = "left";
	}, {
		command = "date +%T";
		font = "fixed";
		location = (0, 0);
		width = 1280;
		align = "middle";
	}, {
		command = "date +%x";
		font = "fixed";
		location = (0, 0);
		max_chars = 100;
		align = "right";
	}
);
```

Setup
===

**Install:**

`git clone https://github.com/tversteeg/xbar && cd xbar && sh setup.sh`

**Uninstall:**

`sudo rm /usr/bin/xbar`
