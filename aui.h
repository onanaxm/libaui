#ifndef __AUI_H__
#define __AUI_H__

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
    const char *anchor;  /* n, ne, e, se, s, sw, w, nw or center */
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
