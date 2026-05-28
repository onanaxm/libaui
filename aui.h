#ifndef __AUI_H__
#define __AUI_H__

#define AUI_WIDGET(widget) (struct aui_widget *) widget

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
    const char *side;
};

struct aui_gridpar {
    unsigned int row;
    unsigned int column;
};

struct aui_widget;
struct aui_window;
struct aui_frame;
struct aui_canvas;      /* rendering free of rules */
struct aui_button;

void aui_run(void);
void aui_free(struct aui_widget *);
struct aui_window *aui_window_new(void);
struct aui_button *aui_button_new(struct aui_widget *);
struct aui_frame *aui_frame_new(struct aui_widget *);
struct aui_canvas *aui_canvas_new(struct aui_widget *);
int aui_place(struct aui_widget *, struct aui_placepar *);
int aui_pack(struct aui_widget *, struct aui_packpar *);
int aui_grid(struct aui_widget *, struct aui_gridpar *);
int aui_getplacepar(struct aui_widget *, struct aui_placepar *);
int aui_getgridpar(struct aui_widget *, struct aui_gridpar *);
int aui_getpackpar(struct aui_widget *, struct aui_packpar *);

#endif
