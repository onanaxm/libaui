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
static struct aui_geometry frame_get_min_size(struct aui_widget *);

static struct widget_ops frame_ops = {
	frame_mouse_hover,
	frame_mouse_unhover,
	frame_mouse_press,
	frame_mouse_release,
	frame_set_geometry,
	frame_free,
	frame_get_min_size,
};

static struct aui_frame_config default_config = {
	.background = { 0x77, 0x77, 0x77, 0xff },
};

struct aui_frame*
aui_frame_new(struct aui_widget *parent, struct aui_frame_config *config)
{
	struct aui_frame *frame = calloc(1, sizeof(struct aui_frame));
	struct aui_widget *widget = (struct aui_widget *)frame;
	struct aui_container *con = (struct aui_container *)frame;

	widget_init(widget, WIDGET_TYPE_FRAME, &frame_ops);

	widget_add(parent, widget);
	con->map_count = 0;

	widget->primitives.count = 1;
	widget->primitives.list = calloc(widget->primitives.count, sizeof(struct primitive *));
	widget->primitives.list[0] = driver->ops->create_rectangle();

	aui_frame_config(widget, config);

	return frame;
}

int
aui_frame_config(struct aui_widget *widget, struct aui_frame_config *config)
{
	struct aui_frame *frame = (struct aui_frame *)widget;

	frame->config = (config == NULL) ? default_config : *config;
	driver->ops->set_rectangle_color(widget->window, widget->primitives.list[0], 
		&frame->config.background);

	return 0;
}

static void
frame_mouse_hover(struct aui_widget *widget, uint16_t x, uint16_t y)
{
	struct aui_container *con = (struct aui_container *)widget;
	struct aui_widget *tohover = NULL;
	struct aui_widget *child;

	TAILQ_FOREACH(child, &widget->queue, entries) {
		if (child->mapped && widget_collides_with_point(child, x, y))
			tohover = child;
	}

	if (tohover != con->focus.hover && con->focus.hover != NULL)
		con->focus.hover->ops->mouse_unhover(con->focus.hover);

	con->focus.hover = tohover;

	if (tohover != NULL)
		tohover->ops->mouse_hover(tohover, x, y);
}

static void
frame_mouse_unhover(struct aui_widget *widget)
{
	struct aui_container *con = (struct aui_container *)widget;

	if (con->focus.hover) {
		con->focus.hover->ops->mouse_unhover(con->focus.hover);
		con->focus.hover = NULL;
	}
}

static void
frame_mouse_press(struct aui_widget *widget, uint16_t x, uint16_t y, uint8_t button)
{
	struct aui_container *con = (struct aui_container *)widget;
	struct aui_widget *topress = NULL;
	struct aui_widget *child;

	TAILQ_FOREACH(child, &widget->queue, entries) {
		if (child->mapped && widget_collides_with_point(child, x, y))
			topress = child;
	}

	if (topress != con->focus.press && con->focus.press != NULL)
		con->focus.press->ops->mouse_release(con->focus.press, x, y, button);

	con->focus.press = topress;

	if (topress != NULL)
		topress->ops->mouse_press(topress, x, y, button);
}

static void
frame_mouse_release(struct aui_widget *widget, uint16_t x, uint16_t y, uint8_t button)
{
	struct aui_container *con = (struct aui_container *)widget;

	if (con->focus.press != NULL) {
		con->focus.press->ops->mouse_release(con->focus.press, x, y, button);
		con->focus.press = NULL;
	}
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

static struct aui_geometry frame_get_min_size(struct aui_widget *widget)
{
	struct aui_geometry geom = { 0, 0, 120, 120 };
	return geom;
}
