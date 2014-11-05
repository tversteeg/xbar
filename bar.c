#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_CONFIG "~/.config/barrc"
#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 16
#define DEFAULT_FONT "-*-terminus-*-r-*-*-16-*-*-*-*-*-*-*"

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
		fprintf(stderr, "Unable to load font \"%s\", using default font\n", font_name);
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
	char config_path[256], font[256], *command, *output;
	int opt, len, verbose, ready, delay, width, height;

	command = output = NULL;
	verbose = ready = 0;
	delay = 1;
	width = DEFAULT_WIDTH;
	height = DEFAULT_HEIGHT;

	while((opt = getopt(argc, argv, "hvd:e:s:p:f:")) != -1){
		switch(opt){
			case 'f':
				if(verbose){
					printf("Font:\t\t\"%s\"\n", optarg);
				}
				strcpy(font, optarg);
				break;
			case 'p':
				if(verbose){
					printf("Config file:\t\"%s\"\n", optarg);
				}
				strcpy(config_path, optarg);
				break;
			case 's':
				if(verbose){
					printf("Size:\t\t\"%s\"\n", optarg);
				}
				sscanf(optarg, "%dx%d", &width, &height);
				break;
			case 'e':
				if(verbose){
					printf("Command:\t\"%s\"\n", optarg);
				}
				command = malloc(strlen(optarg));
				strcpy(command, optarg);
				break;
			case 'd':
				if(verbose){
					printf("Refresh delay:\t%s seconds", optarg);
				}
				delay = atoi(optarg);
				break;
			case 'v':
				verbose = 1;
				break;
			case 'h':
				printf("Usage:\n"
						"\tbar [-h][-v][-d int][-e str]"
						"[-s intxint][-p str][-f str]\n\n"

						"\t[-h]\t\tshow help\n"
						"\t[-v]\t\tshow verbose output\n"
						"\t[-d int]\tset the refresh delay in seconds\n"
						"\t[-e str]\tset the command to execute\n"
						"\t[-s intxint]\tset the width and the height\n"
						"\t[-p str]\tset the path for the config file\n"
						"\t[-f str]\tset the font using the X font style\n"
						);
				return 0;
			case '?':
				return 1;
			default:
				break;
		}
	}

	if(config_path == NULL){
		// Using default directory
		strcpy(config_path, DEFAULT_CONFIG);
	}
	if(font == NULL){
		strcpy(font, DEFAULT_FONT);
	}

	create_bar(width, height, "#CCCCCC", font);

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

	free(command);

	return 0;
}
