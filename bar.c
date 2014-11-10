#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <libconfig.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_COMMAND "echo \"$(date +%T)\"" // Show the time in hh:mm:ss
#define DEFAULT_CONFIG  "xbar.cfg"
#define DEFAULT_X       0
#define DEFAULT_Y       0
#define DEFAULT_WIDTH   800
#define DEFAULT_HEIGHT  16
#define DEFAULT_FONT    "fixed" // Default font
#define DEFAULT_DELAY   1 // 1 second
#define DEFAULT_ALIGN	1 // Middle
#define DEFAULT_FGCOLOR "#FFFFFF"
#define DEFAULT_BGCOLOR "#000000"

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

int draw_bars(int height)
{
	int i, x, y, direction, ascent, descent;
	size_t len;
	char *output;
	XCharStruct overall;

	output = NULL;

	XClearWindow(disp, win);
	for(i = 0; i < bars_len; i++){
		len = sys_output(&output, bars[i].command) - 1;

		XTextExtents(bars[i].font, output, len, &direction, 
				&ascent, &descent, &overall);

		x = bars[i].x;
		y = bars[i].y + (height >> 1) + ((ascent - descent) >> 1);
		if(bars[i].align == 1){
			x = bars[i].x + ((bars[i].width - overall.width) >> 1);
		}else if(bars[i].align == 2){
			x = bars[i]. x + bars[i].width - overall.width;
		}

		XDrawString(disp, win, gc, x, y, output, len);

		free(output);
	}
	XSync(disp, True);

	return 0;
}

int create_bar(int x, int y, int width, int height, char *font_color, char *back_color)
{
	Atom WM_WINDOW_TYPE, WM_WINDOW_TYPE_DOCK;
	XColor back, font;
	Colormap colormap;
	int i, screen;

	disp = XOpenDisplay(NULL);
	screen = DefaultScreen(disp);

	WM_WINDOW_TYPE = XInternAtom(disp, "_NET_WM_WINDOW_TYPE", False);
	WM_WINDOW_TYPE_DOCK = XInternAtom(disp, "_NET_WM_WINDOW_TYPE_DOCK", False);

	colormap = DefaultColormap(disp, 0);
	XParseColor(disp, colormap, font_color, &font);
	XAllocColor(disp, colormap, &font);	
	XParseColor(disp, colormap, back_color, &back);
	XAllocColor(disp, colormap, &back);	

	win = XCreateSimpleWindow(disp, RootWindow(disp, screen), x, y, 
			width, height, 0, font.pixel,  back.pixel);

	XChangeProperty(disp, win, WM_WINDOW_TYPE, XA_ATOM, 32,
			PropModeReplace, (unsigned char*)&WM_WINDOW_TYPE_DOCK, 1);

	XSelectInput(disp, win, ExposureMask);

	XMapWindow(disp, win);
	XFlush(disp);

	gc = XCreateGC(disp, win, 0, 0);
	XSetForeground(disp, gc, font.pixel);
	XSetBackground(disp, gc, back.pixel);

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

int parse_config(char *path)
{
	config_t config;
	config_setting_t *setting, *elem;
	bar_t *bar;
	const char *str;

	int i, len;

	config_init(&config);
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

	config_lookup_string(&config, "foreground", (const char**)&fg_color);
	config_lookup_string(&config, "background", (const char**)&bg_color);

	if(!config_lookup_int(&config, "delay", &delay)){
		delay = DEFAULT_DELAY;
	}
	if(!config_lookup_int(&config, "x", &x)){
		x = DEFAULT_X;
	}
	if(!config_lookup_int(&config, "y", &y)){
		y = DEFAULT_Y;
	}
	if(!config_lookup_int(&config, "width", &width)){
		width = DEFAULT_WIDTH;
	}
	if(!config_lookup_int(&config, "height", &height)){
		height = DEFAULT_HEIGHT;
	}

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

	for(i = 0; i < len; i++){
		bar = bars + i;
		elem = config_setting_get_elem(setting, i);

		if(!config_setting_lookup_string(elem, "command", &str)){
			bar->command = malloc(sizeof(DEFAULT_COMMAND));
			strcpy(bar->command, DEFAULT_COMMAND);
		}else{
			bar->command = malloc(strlen(str));
			strcpy(bar->command, str);
		}
		if(!config_setting_lookup_string(elem, "font", &str)){
			bar->font_name = malloc(sizeof(DEFAULT_FONT));
			strcpy(bar->font_name, DEFAULT_FONT);
		}else{
			bar->font_name = malloc(strlen(str));
			strcpy(bar->font_name, str);
		}
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
		if(!config_setting_lookup_int(elem, "x", &bar->x)){
			bar->x = DEFAULT_X;
		}
		if(!config_setting_lookup_int(elem, "y", &bar->y)){
			bar->y = DEFAULT_Y;
		}
		if(!config_setting_lookup_int(elem, "width", &bar->width)){
			bar->width = DEFAULT_WIDTH;
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

	if(config_path == NULL){
		config_path = malloc(sizeof(DEFAULT_CONFIG));
		strcpy(config_path, DEFAULT_CONFIG);
	}

	fg_color = bg_color = NULL;
	parse_config(config_path);
	free(config_path);

	if(fg_color == NULL){
		fg_color = malloc(sizeof(DEFAULT_FGCOLOR));
		strcpy(fg_color, DEFAULT_FGCOLOR);
	}
	if(bg_color == NULL){
		bg_color = malloc(sizeof(DEFAULT_BGCOLOR));
		strcpy(bg_color, DEFAULT_BGCOLOR);
	}

	create_bar(x, y, width, height, fg_color, bg_color);

	ready = 0;
	while(!ready){
		XEvent e;
		XNextEvent(disp, &e);
		if(e.type == Expose) {
			ready = 1;
		}
	}

	while(1){
		draw_bars(height);
		sleep(delay);
	}

	XFreeGC(disp, gc);
	XUnmapWindow(disp, win);
	XCloseDisplay(disp);

	return 0;
}
