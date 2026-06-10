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
#define AUI_ANCHOR_NW       0
#define AUI_ANCHOR_N        1
#define AUI_ANCHOR_E        2
#define AUI_ANCHOR_SE       3
#define AUI_ANCHOR_S        4
#define AUI_ANCHOR_SW       5
#define AUI_ANCHOR_W        6
#define AUI_ANCHOR_NE       7
#define AUI_ANCHOR_CENTER   8
    unsigned char anchor;
};

struct aui_packpar {
    unsigned char anchor;
#define AUI_FILL_NONE   0
#define AUI_FILL_X      1
#define AUI_FILL_Y      2
#define AUI_FILL_BOTH   3
    unsigned char fill;
#define AUI_SIDE_TOP        1
#define AUI_SIDE_RIGHT      2
#define AUI_SIDE_LEFT       3
#define AUI_SIDE_BOTTOM     4
    unsigned char side;
};

struct aui_gridpar {
    unsigned int column;
    unsigned int row;
};

struct aui_widget;
struct aui_window;
struct aui_frame;
struct aui_canvas;      /* rendering free of rules */
struct aui_button;

struct aui_windowconfig {
    unsigned int width;
    unsigned int height;
    const char *title; /* Should not be allocated, string literal */
};

struct aui_buttonconfig {
    const char *text; /* Should not be allocated, string literal */
};

struct aui_frameconfig {
    struct aui_color bg;
};

void aui_run(void);
void aui_destroy(struct aui_widget *);
struct aui_window *aui_window_new(struct aui_windowconfig *);
struct aui_button *aui_button_new(struct aui_widget *, struct aui_buttonconfig *);
struct aui_frame *aui_frame_new(struct aui_widget *);
struct aui_canvas *aui_canvas_new(struct aui_widget *);
int aui_place(struct aui_widget *, struct aui_placepar *);
int aui_pack(struct aui_widget *, struct aui_packpar *);
int aui_grid(struct aui_widget *, struct aui_gridpar *);
int aui_getplacepar(struct aui_widget *, struct aui_placepar *);
int aui_getgridpar(struct aui_widget *, struct aui_gridpar *);
int aui_getpackpar(struct aui_widget *, struct aui_packpar *);

#endif
