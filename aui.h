#ifndef __AUI_H__
#define __AUI_H__

#define AUI_WIDGET(widget) ((struct aui_widget *) widget)

struct aui_color {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	unsigned char alpha;
};

struct aui_placepar {
	int x, y;
	float relx;
	float rely;
	unsigned int width;
	unsigned int height;
	float relwidth;
	float relheight;
#define AUI_ANCHOR_NW	   	0
#define AUI_ANCHOR_N		1
#define AUI_ANCHOR_E		2
#define AUI_ANCHOR_SE	   	3
#define AUI_ANCHOR_S		4
#define AUI_ANCHOR_SW	   	5
#define AUI_ANCHOR_W		6
#define AUI_ANCHOR_NE	   	7
#define AUI_ANCHOR_CENTER   8
	unsigned char anchor;
};

struct aui_packpar {
	unsigned char anchor;
#define AUI_FILL_NONE   0
#define AUI_FILL_X	  	1
#define AUI_FILL_Y	  	2
#define AUI_FILL_BOTH   3
	unsigned char fill;
#define AUI_SIDE_TOP		0
#define AUI_SIDE_RIGHT	  	1
#define AUI_SIDE_LEFT	   	2
#define AUI_SIDE_BOTTOM	 	3
	unsigned char side;
	unsigned char expand;
};

struct aui_gridpar {
	unsigned int column;
	unsigned int row;
	unsigned int columnspan;
	unsigned int rowspan;
	unsigned int ipadx; /* Internal padding X */
	unsigned int ipady; /* Internal padding Y */
	unsigned int padx;
	unsigned int pady;
};

struct aui_rowpar {
	unsigned int minsize;
	unsigned char weight; /*  Minimum is 1 */
	unsigned char uniform;
	unsigned int pad;
};

struct aui_columnpar {
	unsigned int minsize;
	unsigned char weight; /*  Minimum is 1 */
	unsigned char uniform;
	unsigned int pad;
};

struct aui_widget;
struct aui_window;
struct aui_frame;
struct aui_canvas;	  /* rendering free of rules */
struct aui_button;
struct aui_label;

struct aui_window_config {
	unsigned int width;
	unsigned int height;
	const char *title; /* Not allocated by libaui */
};

struct aui_button_config {
	struct aui_color foreground;
	const char *text; /* Not allocated by libaui */
	void (*command) (struct aui_button *, void *args);
	void *args;
};

struct aui_frame_config {
	struct aui_color background;
};

struct aui_canvas_config {
	struct aui_color background;
};

struct aui_label_config {
	char *text;
	struct aui_color foreground;
	struct aui_color bg;
};

void aui_run(void);
void aui_destroy(struct aui_widget *);
struct aui_window *aui_window_new(struct aui_window_config *);
struct aui_button *aui_button_new(struct aui_widget *, struct aui_button_config *);
int aui_button_config(struct aui_widget *, struct aui_button_config *);
struct aui_frame *aui_frame_new(struct aui_widget *, struct aui_frame_config *);
int aui_frame_config(struct aui_widget *, struct aui_frame_config *);
struct aui_canvas *aui_canvas_new(struct aui_widget *, struct aui_canvas_config *);
int aui_canvas_config(struct aui_widget *, struct aui_canvas_config *);
struct aui_label *aui_label_new(struct aui_widget *, struct aui_label_config *);
int aui_label_config(struct aui_widget *, struct aui_label_config *);
int aui_place(struct aui_widget *, struct aui_placepar *);
int aui_pack(struct aui_widget *, struct aui_packpar *);
int aui_grid(struct aui_widget *, struct aui_gridpar *);
int aui_getplacepar(struct aui_widget *, struct aui_placepar *);
int aui_getgridpar(struct aui_widget *, struct aui_gridpar *);
int aui_getpackpar(struct aui_widget *, struct aui_packpar *);

/*
 * Grid container operations
 */
int aui_rowconfigure(struct aui_widget *, struct aui_rowpar *);
int aui_columnconfigure(struct aui_widget *, struct aui_columnpar *);

#define AUI_CANVAS_RECTANGLE		0
#define AUI_CANVAS_ARC				1
#define AUI_CANVAS_IMAGE			2
#define AUI_CANVAS_BITMAP			3
#define AUI_CANVAS_OVAL				4
#define AUI_CANVAS_POLYGON			5
#define AUI_CANVAS_LINE				6
#define AUI_CANVAS_TEXT				7
#define AUI_CANVAS_WINDOW			8

struct aui_canvas_item;

struct aui_canvas_line {
	int x1, y1;
	int x2, y2;
};

struct aui_canvas_rectangle {
	int x, y;
	struct aui_color color;
	unsigned int width, height;
};

struct aui_canvas_item_config {
	union {
		struct aui_canvas_line line;
		struct aui_canvas_rectangle rectangle;
	};
};

struct aui_canvas_item *aui_canvas_create(struct aui_canvas *, unsigned char,
	struct aui_canvas_item_config *);
void aui_canvas_delete(struct aui_canvas *, struct aui_canvas_item *);

#endif
