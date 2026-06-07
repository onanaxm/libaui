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

static struct widget_ops button_ops = {
    button_mouse_hover,
    button_mouse_unhover,
    button_mouse_press,
    button_mouse_release,
    button_set_geometry, /* set geometry */
    button_free,
};

struct aui_color color[] = {
    { 0xff, 0, 0, 0xff },       /* Borders */
    { 0, 0, 0, 0xff },          /* Normal */
    { 0x1b, 0x1b, 0x1b, 0xff }, /* Hover */
    { 0x3b, 0x3b, 0x3b, 0xff }, /* Press */
};

static struct aui_buttonconfig default_config = {
    .text = NULL,
};

struct aui_button*
aui_button_new(struct aui_widget *parent, struct aui_buttonconfig *config)
{
    struct aui_button *button = calloc(1, sizeof(struct aui_button));
	struct aui_widget *widget = &button->widget;
    widget_init(&button->widget);
    widget->in_ops = &button_ops;
    widget->type = WIDGET_TYPE_BUTTON;

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

    driver->ops->set_rectangle_color(widget->window, widget->primitives.list[0], &color[0]);
    driver->ops->set_rectangle_color(widget->window, widget->primitives.list[1], &color[1]);

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
}

static void
button_set_geometry(struct aui_widget *widget, struct aui_geometry *geom)
{
    struct aui_button *button = (struct aui_button *)widget;
    struct aui_geometry fgeom = { .x = geom->x + 1,
                                  .y = geom->y + 1,
                                  .width =  geom->width - 2,
                                  .height = geom->height - 2 };
    struct aui_geometry tgeom = driver->ops->get_text_geometry(widget->primitives.list[2]);
    int tx = geom->x + geom->width / 2 - tgeom.width / 2;
    int ty = geom->y + geom->height / 2 + tgeom.height / 2;

    memcpy(&widget->geom, geom, sizeof(struct aui_geometry));
    driver->ops->set_rectangle_geometry(widget->window, widget->primitives.list[0], geom);
    driver->ops->set_rectangle_geometry(widget->window, widget->primitives.list[1], &fgeom);
    driver->ops->set_text(widget->primitives.list[2], button->config.text, tx, ty);
}
