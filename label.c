#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver.h"
#include "widget.h"

static void label_mouse_hover(struct aui_widget *, uint16_t, uint16_t);
static void label_mouse_unhover(struct aui_widget *);
static void label_mouse_press(struct aui_widget *, uint16_t, uint16_t, uint8_t);
static void label_mouse_release(struct aui_widget *, uint16_t, uint16_t, uint8_t);
static void label_set_geometry(struct aui_widget *, struct aui_geometry *);
static void label_free(struct aui_widget *);
static struct aui_geometry label_get_min_size(struct aui_widget *);

static struct widget_ops label_ops = {
	label_mouse_hover,
	label_mouse_unhover,
	label_mouse_press,
	label_mouse_release,
	label_set_geometry, /* set geometry */
	label_free,
	label_get_min_size,
};

struct aui_label_config default_config = {
	.text = "Hello AUI",
	.foreground = { 0, 0, 0, 255 },
};

struct aui_label*
aui_label_new(struct aui_widget *parent, struct aui_label_config *config)
{
	struct aui_label *label = calloc(1, sizeof(struct aui_label));
	struct aui_widget *widget = &label->widget;

	widget_init(widget, WIDGET_TYPE_LABEL, &label_ops);
	widget_add(parent, widget);

	widget->primitives.count = 1;
	widget->primitives.list = calloc(widget->primitives.count, sizeof(struct primitive*));

	widget->primitives.list[0] = driver->ops->create_text();
	aui_label_config(widget, config);

	return label;
}

int
aui_label_config(struct aui_widget *widget, struct aui_label_config *config)
{
	struct aui_label *label = (struct aui_label *)widget;
	struct aui_geometry geom = widget->geom;

	if (label->config.text) {
		free(label->config.text);
		label->config.text = NULL;
	}

	config = (config == NULL) ? &default_config : config;
	label->config = *config;

	if (config->text)
		label->config.text = strdup(config->text);

	driver->ops->set_text(widget->primitives.list[0], label->config.text, 0, 0);
	driver->ops->set_text_color(widget->primitives.list[0], &label->config.foreground);
	label_set_geometry(widget, &geom);

	layout_organize(widget->parent);
	return 0;
}

static void
label_mouse_hover(struct aui_widget *widget, uint16_t dx, uint16_t dy)
{

}

static void
label_mouse_unhover(struct aui_widget *widget)
{

}

static void
label_mouse_press(struct aui_widget *widget, uint16_t x, uint16_t y, uint8_t button)
{

}

static void
label_mouse_release(struct aui_widget *widget, uint16_t x, uint16_t y, uint8_t button)
{

}

static void
label_set_geometry(struct aui_widget *widget, struct aui_geometry *geom)
{
	struct aui_geometry tgeom = driver->ops->get_text_geometry(widget->primitives.list[0]);
	struct aui_label *label = (struct aui_label *)widget;

	driver->ops->set_text(
		widget->primitives.list[0], 
		label->config.text, 
		geom->x + geom->width / 2 - tgeom.width / 2 ,
		geom->y + geom->height
	);
	widget->geom = *geom;
}

static void
label_free(struct aui_widget *widget)
{

}

static struct aui_geometry
label_get_min_size(struct aui_widget *widget)
{
	struct aui_geometry geom = driver->ops->get_text_geometry(widget->primitives.list[0]);
	return geom;
}
