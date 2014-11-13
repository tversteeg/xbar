#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <libconfig.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* The command to execute, this is the text that gets displayed on the bar */
#define DEFAULT_COMMAND "echo \"$(date +%T)\""
/* The location of the configuration file */
#define DEFAULT_CONFIG  "xbar.cfg"
/* The x location of the bar */
#define DEFAULT_X       0
/* The y location of the bar */
#define DEFAULT_Y       0
/* The width of the bar */
#define DEFAULT_WIDTH   800
/* The height of the bar */
#define DEFAULT_HEIGHT  16
/* The x11 font to use, fixed is the systems default font */
#define DEFAULT_FONT    "fixed"
/* The delay in seconds */
#define DEFAULT_DELAY   1
/* The alignment of the textfield, possible values:
 * 0: left
 * 1: middle
 * 2: right */
#define DEFAULT_ALIGN	1
/* The color of the text */
#define DEFAULT_FGCOLOR "#FFFFFF"
/* The color of the bar */
#define DEFAULT_BGCOLOR "#000000"

/* The structure which resembles a textfield */
typedef struct {
	char *command, *font_name;
	XFontStruct *font;
	int x, y, width;
	char align;
} bar_t;

Window win;
Display *disp;
GC gc;
bar_t *bars;
size_t bars_len;
char *fg_color, *bg_color;
int delay, x, y, width, height;

/* Execute the command to get the output in the stdin as a string */
int sys_output(char **buf, char *command)
{
	FILE *fp;
	char output[1035];
	int size;

	/* Execute the command */
	fp = popen(command, "r");
	if(fp == NULL){
		fprintf(stderr, "Failed to run command: %s\n", command);
		exit(1);
	}

	/* Read the output from the command's filepointer */
	fgets(output, sizeof(output) - 1, fp);

	/* Copy the command into the buffer */
	size = strlen(output);
	*buf = malloc(size);
	strcpy(*buf, output);
	(*buf)[size - 1] = '\0';

	fclose(fp);

	return size;
}

/* The update function to draw the textfields */
int draw_bars(int height)
{
	int i, x, y, direction, ascent, descent;
	size_t len;
	char *output;
	XCharStruct overall;

	output = NULL;

	/* Clear the old textfields */
	XClearWindow(disp, win);
	for(i = 0; i < bars_len; i++){
		/* Get the return value of the command */
		len = sys_output(&output, bars[i].command) - 1;

		/* Get the text geometry */
		XTextExtents(bars[i].font, output, len, &direction, 
				&ascent, &descent, &overall);

		/* Set the text position according to alignment */
		x = bars[i].x;
		y = bars[i].y + (height >> 1) + ((ascent - descent) >> 1);
		if(bars[i].align == 1){
			x = bars[i].x + ((bars[i].width - overall.width) >> 1);
		}else if(bars[i].align == 2){
			x = bars[i]. x + bars[i].width - overall.width;
		}

		/* Draw the textfield */
		XDrawString(disp, win, gc, x, y, output, len);

		free(output);
	}
	/* Sync the display to show it correctly */
	XSync(disp, True);

	return 0;
}

/* Create the background bar using xlib and load the fonts */
int create_bar(int x, int y, int width, int height, char *font_color, char *back_color)
{
	Atom WM_WINDOW_TYPE, WM_WINDOW_TYPE_DOCK;
	XColor back, font;
	Colormap colormap;
	int i, screen;

	/* Open the default display */
	disp = XOpenDisplay(NULL);
	screen = DefaultScreen(disp);

	/* Load the atoms to set the window type to
	 * This makes sure no decoration is drawn */
	WM_WINDOW_TYPE = XInternAtom(disp, "_NET_WM_WINDOW_TYPE", False);
	WM_WINDOW_TYPE_DOCK = XInternAtom(disp, "_NET_WM_WINDOW_TYPE_DOCK", False);

	/* Load the colors as xlib values */
	colormap = DefaultColormap(disp, 0);
	XParseColor(disp, colormap, font_color, &font);
	XAllocColor(disp, colormap, &font);	
	XParseColor(disp, colormap, back_color, &back);
	XAllocColor(disp, colormap, &back);	

	/* Create the bar background */
	win = XCreateSimpleWindow(disp, RootWindow(disp, screen), x, y, 
			width, height, 0, font.pixel,  back.pixel);

	/* Set it to the window type with the atoms from above */
	XChangeProperty(disp, win, WM_WINDOW_TYPE, XA_ATOM, 32,
			PropModeReplace, (unsigned char*)&WM_WINDOW_TYPE_DOCK, 1);

	/* It only reacts to exposure events */
	XSelectInput(disp, win, ExposureMask);

	/* Actually display the bar */
	XMapWindow(disp, win);
	XFlush(disp);

	/* Create a drawing area for the textfields */
	gc = XCreateGC(disp, win, 0, 0);
	XSetForeground(disp, gc, font.pixel);
	XSetBackground(disp, gc, back.pixel);

	/* Load each font from each textfield */
	for(i = 0; i < bars_len; i++){
		bars[i].font = XLoadQueryFont(disp, bars[i].font_name);
		if(!bars[i].font){
			fprintf(stderr, "Unable to load font \"%s\", using default font\n", bars[i].font_name);
			bars[i].font = XLoadQueryFont(disp, "fixed");
		}
		XSetFont(disp, gc, bars[i].font->fid);
	}

	return 0;
}

/* Try to load the libconfig style configuration file and parse it */
int parse_config(char *path)
{
	config_t config;
	config_setting_t *setting, *elem;
	bar_t *bar;
	const char *str;

	int i, len;

	config_init(&config);
	/* Try to open the file and use the default settings when it fails */
	if(!config_read_file(&config, path)){
		fprintf(stderr, "Error in loading config \"%s\" on line %d: %s\n"
				"Using default settings\n", path,
				config_error_line(&config), config_error_text(&config));
		config_destroy(&config);

		bars = malloc(sizeof(bar_t));
		bars_len = 1;
		bars->command = malloc(sizeof(DEFAULT_COMMAND));
		strcpy(bars->command, DEFAULT_COMMAND);
		bars->font_name = malloc(sizeof(DEFAULT_FONT));
		strcpy(bars->font_name, DEFAULT_FONT);
		bars->align = DEFAULT_ALIGN;
		bars->x = DEFAULT_X;
		bars->y = DEFAULT_Y;
		bars->width = DEFAULT_WIDTH;

		return -1;
	}

	/* Try to load the color of the fonts */
	if(!config_lookup_string(&config, "foreground", &str)){
		fg_color = malloc(sizeof(DEFAULT_FGCOLOR));
		strcpy(fg_color, DEFAULT_FGCOLOR);
	}else{
		fg_color = malloc(strlen(str));
		strcpy(fg_color, str);
	}
	/* Try to load the color of the background of the bar */
	if(!config_lookup_string(&config, "background", &str)){
		bg_color = malloc(sizeof(DEFAULT_BGCOLOR));
		strcpy(bg_color, DEFAULT_BGCOLOR);
	}else{
		bg_color = malloc(strlen(str));
		strcpy(bg_color, str);
	}

	/* Try to load the delay the bar text refreshes in seconds */
	if(!config_lookup_int(&config, "delay", &delay)){
		delay = DEFAULT_DELAY;
	}
	/* Try to load the x position of the bar */
	if(!config_lookup_int(&config, "x", &x)){
		x = DEFAULT_X;
	}
	/* Try to load the y position of the bar */
	if(!config_lookup_int(&config, "y", &y)){
		y = DEFAULT_Y;
	}
	/* Try to load the width of the bar */
	if(!config_lookup_int(&config, "width", &width)){
		width = DEFAULT_WIDTH;
	}
	/* Try to load the height of the bar */
	if(!config_lookup_int(&config, "height", &height)){
		height = DEFAULT_HEIGHT;
	}

	/* Try to load the text commands/settings */
	setting = config_lookup(&config, "text");
	if(setting == NULL){
		fprintf(stderr, "No \"text\" field in the config supplied,"
				" using default clock\n");
		config_destroy(&config);

		bars = malloc(sizeof(bar_t));
		bars_len = 1;
		bars->command = malloc(sizeof(DEFAULT_COMMAND));
		strcpy(bars->command, DEFAULT_COMMAND);
		bars->font_name = malloc(sizeof(DEFAULT_FONT));
		strcpy(bars->font_name, DEFAULT_FONT);

		return -1;
	}

	len = config_setting_length(setting);
	bars = malloc(sizeof(bar_t) * len);
	bars_len = len;
	/* Try to load and loop through the different textfields */
	for(i = 0; i < len; i++){
		bar = bars + i;
		elem = config_setting_get_elem(setting, i);

		/* Try to load the command to execute of the textfield */
		if(!config_setting_lookup_string(elem, "command", &str)){
			bar->command = malloc(sizeof(DEFAULT_COMMAND));
			strcpy(bar->command, DEFAULT_COMMAND);
		}else{
			bar->command = malloc(strlen(str));
			strcpy(bar->command, str);
		}
		/* Try to load the X11 style font of the textfield */
		if(!config_setting_lookup_string(elem, "font", &str)){
			bar->font_name = malloc(sizeof(DEFAULT_FONT));
			strcpy(bar->font_name, DEFAULT_FONT);
		}else{
			bar->font_name = malloc(strlen(str));
			strcpy(bar->font_name, str);
		}
		/* Try to load the alignment of the textfield */
		if(!config_setting_lookup_string(elem, "align", &str)){
			bar->align = DEFAULT_ALIGN;
		}else{
			if(strcmp(str, "left") == 0){
				bar->align = 0;
			}else if(strcmp(str, "middle") == 0){
				bar->align = 1;
			}else{
				bar->align = 2;
			}
		}
		/* Try to load the x position of the textfield */
		if(!config_setting_lookup_int(elem, "x", &bar->x)){
			bar->x = DEFAULT_X;
		}
		/* Try to load the y position of the textfield */
		if(!config_setting_lookup_int(elem, "y", &bar->y)){
			bar->y = DEFAULT_Y;
		}
		/* Try to load the maximum width of the textfield */
		if(!config_setting_lookup_int(elem, "width", &bar->width)){
			bar->width = width;
		}
	}

	config_destroy(&config);

	return 0;
}

int main(int argc, char **argv)
{
	char *config_path;
	int opt, ready;

	y = DEFAULT_Y;
	width = DEFAULT_WIDTH;
	height = DEFAULT_HEIGHT;

	config_path = NULL;
	/* Loop through the arguments to display helpfiles or save the configuration file's path */
	while((opt = getopt(argc, argv, "p:h")) != -1){
		switch(opt){
			case 'p':
				config_path = malloc(strlen(optarg));
				strcpy(config_path, optarg);
				break;
			case 'h':
				printf("Usage:\n"
						"\txbar [-h][-s filename]\n\n"

						"\t[-h]\t\tshow help\n"
						"\t[-p filename]\tset the config file\n"
					  );
				return 0;
			case '?':
				return 1;
			default:
				break;
		}
	}

	/* Use default path if none is supplied */
	if(config_path == NULL){
		config_path = malloc(sizeof(DEFAULT_CONFIG));
		strcpy(config_path, DEFAULT_CONFIG);
	}

	/* Set the configuration constants and populate the textfields */
	parse_config(config_path);
	free(config_path);

	/* Create the bar using xlib and load the font */
	create_bar(x, y, width, height, fg_color, bg_color);

	ready = 0;
	/* Watch for the first Expose event so the bar can be drawn */
	while(!ready){
		XEvent e;
		XNextEvent(disp, &e);
		if(e.type == Expose) {
			ready = 1;
		}
	}

	/* Redraw the bar with an interval set by delay */
	while(1){
		draw_bars(height);
		sleep(delay);
	}

	/* Free the xlib values */
	XFreeGC(disp, gc);
	XUnmapWindow(disp, win);
	XCloseDisplay(disp);

	return 0;
}
