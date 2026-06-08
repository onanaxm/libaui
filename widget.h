#ifndef __WIDGET_H__
#define __WIDGET_H__

#include "openbsd-queue.h"

#include "layout.h"

/*
 * Gives screen position
 */
struct aui_geometry {
    int x, y;
    uint16_t width;
    uint16_t height;
};

enum widget_type {
    WIDGET_TYPE_WINDOW,
    WIDGET_TYPE_FRAME,
    WIDGET_TYPE_CANVAS,
	WIDGET_TYPE_BUTTON,
};

TAILQ_HEAD(aui_widget_queue, aui_widget);
struct aui_widget {
    enum widget_type type;
    struct aui_geometry geom;

	struct aui_window *window;
    struct widget_ops *in_ops;

    struct aui_placepar placepar;
    struct aui_gridpar gridpar;
    struct aui_packpar packpar;

    unsigned int mapped;

    struct widget_primitives {
        unsigned int count;
        struct primitive **list;
    } primitives;

    struct aui_widget *parent;
    struct aui_widget_queue queue;
    TAILQ_ENTRY(aui_widget) entries;
};

struct widget_ops {
    void (*mouse_hover) (struct aui_widget *, uint16_t, uint16_t);
    void (*mouse_unhover) (struct aui_widget *);
    void (*mouse_press) (struct aui_widget *, uint16_t, uint16_t, uint8_t);
    void (*mouse_release) (struct aui_widget *, uint16_t, uint16_t, uint8_t);
    void (*set_geometry) (struct aui_widget *, struct aui_geometry *);
    void (*free) (struct aui_widget *);
    struct aui_geometry (*get_min_size) (struct aui_widget *);
};

struct aui_container {
    struct aui_widget widget;
    enum aui_layout_type layout_type;
    uint32_t grid_size[2];

    struct container_focus {
        struct aui_widget *press;
        struct aui_widget *hover;
    } focus;
};

LIST_HEAD(aui_window_list, aui_window);
struct aui_window {
    struct aui_container con;
    const char *title;
    int draw_flag;
    struct aui_windowconfig config;

    LIST_ENTRY(aui_window) entries;
};

struct aui_frame {
    struct aui_container con;
};

struct aui_canvas {
    struct aui_container con;
};

struct aui_button {
	struct aui_widget widget;
    unsigned int pressed;
    struct aui_buttonconfig config;
};

extern struct aui_window_list wlist;

void widget_init(struct aui_widget *);
void widget_draw(struct aui_widget *);
void widget_add(struct aui_widget *, struct aui_widget *);
void widget_remove(struct aui_widget *, struct aui_widget *);
int widget_collides_with_point(struct aui_widget*, uint16_t, uint16_t);

#endif
