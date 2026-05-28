#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "driver.h"
#include "widget.h"

static void frame_mouse_hover(struct aui_widget *, uint16_t, uint16_t);
static void frame_mouse_unhover(struct aui_widget *);
static void frame_mouse_press(struct aui_widget *, uint16_t, uint16_t, uint8_t);
static void frame_mouse_release(struct aui_widget *, uint16_t, uint16_t, uint8_t);
static void frame_set_geometry(struct aui_widget *, struct aui_geometry *);
static void frame_free(struct aui_widget *);

static struct widget_ops frame_ops = {
    frame_mouse_hover,
    frame_mouse_unhover,
    frame_mouse_press,
    frame_mouse_release,
    frame_set_geometry,
    frame_free,
};

struct aui_color colors[] = {
    { 0x77, 0x77, 0x77, 0xff },
};

struct aui_frame*
aui_frame_new(struct aui_widget *parent)
{
    struct aui_frame *frame = calloc(1, sizeof(struct aui_frame));
    struct aui_widget *widget = (struct aui_widget *)frame;

    widget_init(widget);
    widget->in_ops = &frame_ops;
    widget->type = WIDGET_TYPE_FRAME;

    widget_add(parent, widget);

    widget->primitives.count = 1;
    widget->primitives.list = calloc(widget->primitives.count, sizeof(struct primitive *));
    widget->primitives.list[0] = driver->ops->create_rectangle();

    driver->ops->set_rectangle_color(widget->window, widget->primitives.list[0], &colors[0]);

    return frame;
}

static void
frame_mouse_hover(struct aui_widget *widget, uint16_t x, uint16_t y)
{

}

static void
frame_mouse_unhover(struct aui_widget *widget)
{

}

static void
frame_mouse_press(struct aui_widget *widget, uint16_t x, uint16_t y, uint8_t button)
{

}

static void
frame_mouse_release(struct aui_widget *widget, uint16_t x, uint16_t y, uint8_t button)
{

}

static void
frame_set_geometry(struct aui_widget *widget, struct aui_geometry *geom)
{
    memcpy(&widget->geom, geom, sizeof(struct aui_geometry));
    driver->ops->set_rectangle_geometry(widget->window, widget->primitives.list[0], geom);
}

static void
frame_free(struct aui_widget *widget)
{

}
