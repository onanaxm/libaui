/*
 * Canvases do not store pixels but rather the operations taking
 * place within them. This makes resizing easier.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver.h"
#include "widget.h"

static void canvas_mouse_hover(struct aui_widget *, uint16_t, uint16_t);
static void canvas_mouse_unhover(struct aui_widget *);
static void canvas_mouse_press(struct aui_widget *, uint16_t, uint16_t, uint8_t);
static void canvas_mouse_release(struct aui_widget *, uint16_t, uint16_t, uint8_t);
static void canvas_set_geometry(struct aui_widget *, struct aui_geometry *);
static void canvas_free(struct aui_widget *);
static struct aui_geometry canvas_get_min_size(struct aui_widget *);

#define DEFAULT_W 300
#define DEFAULT_H 200

static struct widget_ops canvas_ops = {
	canvas_mouse_hover,
	canvas_mouse_unhover,
	canvas_mouse_press,
	canvas_mouse_release,
	canvas_set_geometry, /* set geometry */
	canvas_free,
	canvas_get_min_size,
};

static struct aui_color colors[] = {
	{ 0xff, 0xff, 0xff, 0xff },
};

struct aui_canvas*
aui_canvas_new(struct aui_widget *parent, struct aui_canvas_config *config)
{
	struct aui_canvas *canvas = calloc(1, sizeof(struct aui_canvas));
	struct aui_widget *widget = (struct aui_widget *)canvas;

	widget_init(widget, WIDGET_TYPE_CANVAS, &canvas_ops);

	widget_add(parent, widget);
	widget->primitives.count = 1;
	widget->primitives.list = calloc(widget->primitives.count, sizeof(struct primitive *));
	widget->primitives.list[0] = driver->ops->create_rectangle();

	driver->ops->set_rectangle_color(widget->window, widget->primitives.list[0], &colors[0]);

	TAILQ_INIT(&canvas->queue);

	return canvas;
}

struct aui_canvas_item*
aui_canvas_create(struct aui_canvas *canvas, unsigned char type, struct aui_canvas_item_config *cfg)
{
	struct aui_canvas_item *i = calloc(1, sizeof(struct aui_canvas_item));
	struct aui_widget *widget = (struct aui_widget *)canvas;

	if (i == NULL)
		return NULL;

	i->config = *cfg;

	switch (type) {
	case AUI_CANVAS_RECTANGLE:
		i->primitives.count = 1;
		i->primitives.list = calloc(widget->primitives.count, sizeof(struct primitives *));
		i->primitives.list[0] = driver->ops->create_rectangle();

		driver->ops->set_rectangle_color(widget->window, i->primitives.list[0],
			&i->config.rectangle.color);
		break;
	default:
		fprintf(stderr, "libaui: uknwon canvas item type\n");
		free(i);
		return NULL;
	}

	TAILQ_INSERT_TAIL(&canvas->queue, i, entries);

	return i;
}

static void
canvas_mouse_hover(struct aui_widget *widget, uint16_t dx, uint16_t dy)
{

}

static void
canvas_mouse_unhover(struct aui_widget *widget)
{

}

static void
canvas_mouse_press(struct aui_widget *widget, uint16_t x, uint16_t y, uint8_t button)
{

}

static void
canvas_mouse_release(struct aui_widget *widget, uint16_t x, uint16_t y, uint8_t button)
{

}

static void
canvas_set_geometry(struct aui_widget *widget, struct aui_geometry *geom)
{
	struct aui_canvas *canvas = (struct aui_canvas *)widget;
	struct aui_canvas_item *item;

	memcpy(&widget->geom, geom, sizeof(struct aui_geometry));
	driver->ops->set_rectangle_geometry(widget->window, widget->primitives.list[0], geom);

	TAILQ_FOREACH(item, &canvas->queue, entries) {
		struct primitive *prim;

		switch (item->type) {
		case AUI_CANVAS_RECTANGLE: {
			struct aui_geometry g = {
				geom->x + item->config.rectangle.x,
				geom->y + item->config.rectangle.y,
				item->config.rectangle.width,
				item->config.rectangle.height
			};

			prim = item->primitives.list[0];
			driver->ops->set_rectangle_geometry(
				widget->window,
				prim,
				&g
			);
			break;
		}
		default:
			break;
		}
	}
}

static void
canvas_free(struct aui_widget *widget)
{

}

static struct aui_geometry
canvas_get_min_size(struct aui_widget *widget)
{
	struct aui_geometry geom = { .x= 0, .y = 0, .width = DEFAULT_W, .height = DEFAULT_H };
	return geom;
}
