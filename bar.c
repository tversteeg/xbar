#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

Window win;
Display *disp;
GC gc;
XFontStruct *font;

int draw_bar(int width, int height, char *text, int len)
{
	int x, y, direction, ascent, descent;
	XCharStruct overall;

	XTextExtents(font, text, len, &direction, &ascent, &descent, &overall);

	x = (width - overall.width) >> 1;
	y = (height >> 1) + ((ascent - descent) >> 1);

	XClearWindow(disp, win);
	XDrawString(disp, win, gc, x, y, text, len);
	XSync(disp, True);

	return 0;
}

int create_bar(int width, int height, char *back_color, char *font_name)
{
	Atom WM_WINDOW_TYPE, WM_WINDOW_TYPE_DOCK;
	XColor color;
	Colormap colormap;
	int screen;

	disp = XOpenDisplay(NULL);
	screen = DefaultScreen(disp);

	WM_WINDOW_TYPE = XInternAtom(disp, "_NET_WM_WINDOW_TYPE", False);
	WM_WINDOW_TYPE_DOCK = XInternAtom(disp, "_NET_WM_WINDOW_TYPE_DOCK", False);

	colormap = DefaultColormap(disp, 0);
	XParseColor(disp, colormap, back_color, &color);
	XAllocColor(disp, colormap, &color);	

	win = XCreateSimpleWindow(disp, RootWindow(disp, screen), 0, 0, 
			width, height, 0, WhitePixel(disp, 0),  color.pixel);

	XChangeProperty(disp, win, WM_WINDOW_TYPE, XA_ATOM, 32,
			PropModeReplace, (unsigned char*)&WM_WINDOW_TYPE_DOCK, 1);

	XSelectInput(disp, win, ExposureMask);

	XMapWindow(disp, win);
	XFlush(disp);

	gc = XCreateGC(disp, win, 0, 0);
	XSetBackground(disp, gc, color.pixel);
	XSetForeground(disp, gc, BlackPixel(disp, 0));

	font = XLoadQueryFont(disp, font_name);
	if(!font){
		fprintf(stderr, "Unable to load font %s, using fixed\n", font_name);
		font = XLoadQueryFont(disp, "fixed");
	}
	XSetFont(disp, gc, font->fid);

	return 0;
}

int sys_output(char **buf, char *command)
{
	FILE *fp;
	char output[1035];
	int size;

	fp = popen(command, "r");
	if(fp == NULL){
		fprintf(stderr, "Failed to run command: %s\n", command);
		exit(1);
	}

	fgets(output, sizeof(output) - 1, fp);

	size = strlen(output);
	*buf = malloc(size);
	strcpy(*buf, output);
	(*buf)[size - 1] = '\0';

	fclose(fp);

	return size;
}

int main(int argc, char **argv)
{
	char config_path[256] = "~/.config/barrc";
	char *command = NULL, *output = NULL;
	int opt, len, verbose = 0, ready = 0, delay = 1;
	int width = 800, height = 16;

	while((opt = getopt(argc, argv, "vd:c:s:p:")) != -1){
		switch(opt){
			case 'p':
				strcpy(config_path, optarg);
				break;
			case 's':
				sscanf(optarg, "%dx%d", &width, &height);
				break;
			case 'c':
				command = malloc(strlen(optarg));
				strcpy(command, optarg);
				break;
			case 'd':
				delay = atoi(optarg);
				break;
			case 'v':
				verbose = 1;
				break;
			case '?':
				return 1;
			default:
				break;
		}
	}

	if(verbose){
		printf("Reading configuration file: \"%s\"\n", config_path);
	}

	if(command){
		printf("Performing command: \"%s\"\n", command);
	}

	create_bar(width, height, "#CCCCCC", "-*-terminus-*-r-*-*-16-*-*-*-*-*-*-*");

	while(!ready){
		XEvent e;
		XNextEvent(disp, &e);
		if(e.type == Expose) {
			ready = 1;
		}
	}

	while(1){
		len = sys_output(&output, command);
		draw_bar(width, height, output, len - 1);
		free(output);
		sleep(delay);
	}

	XFreeGC(disp, gc);
	XUnmapWindow(disp, win);
	XCloseDisplay(disp);

	return 0;
}
