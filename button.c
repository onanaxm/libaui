#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver.h"
#include "widget.h"

static void button_mouse_hover(struct aui_widget *, uint16_t, uint16_t);
static void button_mouse_unhover(struct aui_widget *);
static void button_mouse_press(struct aui_widget *, uint16_t, uint16_t, uint8_t);
static void button_mouse_release(struct aui_widget *, uint16_t, uint16_t, uint8_t);
static void button_set_geometry(struct aui_widget *, struct aui_geometry *);
static void button_free(struct aui_widget *);
static struct aui_geometry button_get_min_size(struct aui_widget *);

static struct widget_ops button_ops = {
	button_mouse_hover,
	button_mouse_unhover,
	button_mouse_press,
	button_mouse_release,
	button_set_geometry, /* set geometry */
	button_free,
	button_get_min_size,
};

struct aui_color color[] = {
	{ 0xff, 0, 0, 0xff },	   /* Borders */
	{ 0, 0, 0, 0xff },		  /* Normal */
	{ 0x1b, 0x1b, 0x1b, 0xff }, /* Hover */
	{ 0x3b, 0x3b, 0x3b, 0xff }, /* Press */
};

static struct aui_button_config default_config = {
	.text = NULL,
	.foreground = { 0xff, 0, 0, 0xff }
};

struct aui_button*
aui_button_new(struct aui_widget *parent, struct aui_button_config *config)
{
	struct aui_button *button = calloc(1, sizeof(struct aui_button));
	struct aui_widget *widget = &button->widget;
	widget_init(&button->widget, WIDGET_TYPE_BUTTON, &button_ops);

	/*
	 * Creating the rectangle primitive
	 */
	widget_add(parent, widget);
	button->config = (config == NULL) ? default_config : *config;

	widget->primitives.count = 3;
	widget->primitives.list = calloc(widget->primitives.count, sizeof(struct primitive*));
	widget->primitives.list[0] = driver->ops->create_rectangle();
	widget->primitives.list[1] = driver->ops->create_rectangle();
	widget->primitives.list[2] = driver->ops->create_text();

	driver->ops->set_rectangle_color(widget->window, widget->primitives.list[0], 
		&button->config.foreground);
	driver->ops->set_rectangle_color(widget->window, widget->primitives.list[1], &color[1]);
	driver->ops->set_text(widget->primitives.list[2], button->config.text, 0, 0);
	driver->ops->set_text_color(widget->primitives.list[2], &button->config.foreground);

	return button;
}

void
button_free(struct aui_widget *widget)
{
	struct aui_button *button = (struct aui_button *)widget;
	widget_remove(widget->parent, widget);

	driver->ops->delete_rectangle(widget->primitives.list[0]);
	driver->ops->delete_rectangle(widget->primitives.list[1]);

	widget->window->draw_flag = 1;

	free(button);
}

static void
button_mouse_hover(struct aui_widget *widget, uint16_t x, uint16_t y)
{
	struct aui_button *btn = (struct aui_button *)widget;
	if (btn->pressed == 0) {
		driver->ops->set_rectangle_color(widget->window, widget->primitives.list[1], &color[2]);
	} else {
		driver->ops->set_rectangle_color(widget->window, widget->primitives.list[1], &color[3]);
	}
}

static void
button_mouse_unhover(struct aui_widget *widget)
{
	driver->ops->set_rectangle_color(widget->window, widget->primitives.list[1], &color[1]);
}

static void 
button_mouse_press(struct aui_widget *widget, uint16_t x, uint16_t y, uint8_t button)
{
	struct aui_button *btn = (struct aui_button *)widget;

	btn->pressed = 1;
	driver->ops->set_rectangle_color(widget->window, widget->primitives.list[1], &color[3]);
}

static void 
button_mouse_release(struct aui_widget *widget, uint16_t x, uint16_t y, uint8_t button)
{
	struct aui_button *btn = (struct aui_button *)widget;
	btn->pressed = 0;

	driver->ops->set_rectangle_color(widget->window, widget->primitives.list[1], &color[1]);

	if (btn->config.command && widget_collides_with_point(widget, x, y))
		 btn->config.command(btn, btn->config.args);
}

static void
button_set_geometry(struct aui_widget *widget, struct aui_geometry *geom)
{
	struct aui_button *button = (struct aui_button *)widget;
	struct aui_geometry foregroundeom = { 0 };
	struct aui_geometry tgeom = driver->ops->get_text_geometry(widget->primitives.list[2]);
	int tx = geom->x + geom->width / 2 - tgeom.width / 2;
	int ty = geom->y + geom->height / 2 + tgeom.height / 2;

	foregroundeom.x = geom->x + 1;
	foregroundeom.y = geom->y + 1;
	foregroundeom.width = geom->width - 2;
	foregroundeom.height = geom->height - 2;

	memcpy(&widget->geom, geom, sizeof(struct aui_geometry));
	driver->ops->set_rectangle_geometry(widget->window, widget->primitives.list[0], geom);
	driver->ops->set_rectangle_geometry(widget->window, widget->primitives.list[1], &foregroundeom);
	driver->ops->set_text(widget->primitives.list[2], button->config.text, tx, ty);
}

static struct aui_geometry 
button_get_min_size(struct aui_widget *widget)
{
	struct aui_geometry geom = driver->ops->get_text_geometry(widget->primitives.list[2]);
	struct aui_button *button = (struct aui_button *)widget;

	geom.width = (geom.width < 30) ? geom.width = 30 : geom.width + 30;
	geom.height = (button->config.text == NULL) ? 12 + 20 : geom.height + 20;

	return geom;
}
