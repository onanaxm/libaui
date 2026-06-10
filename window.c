#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "event.h"
#include "driver.h"
#include "widget.h"
#include "aui_priv.h"

struct aui_window_list wlist;

static void window_mouse_hover(struct aui_widget *, uint16_t, uint16_t);
static void window_mouse_unhover(struct aui_widget *);
static void window_mouse_press(struct aui_widget *, uint16_t, uint16_t, uint8_t);
static void window_mouse_release(struct aui_widget *, uint16_t, uint16_t, uint8_t);
static void window_free(struct aui_widget *);
static struct aui_geometry window_get_min_size(struct aui_widget *);

static struct widget_ops window_ops = {
    window_mouse_hover,
    window_mouse_unhover,
    window_mouse_press,
    window_mouse_release,
    NULL, /* set geomtry */
    window_free,
    window_get_min_size,
};

static struct aui_windowconfig default_config = {
    .width =    200,
    .height =   200,
    .title =   "aui",
};

struct aui_window*
aui_window_new(struct aui_windowconfig *config)
{
    if (driver == NULL) {
        if (driver_open(DRIVER_TYPE_X11) == -1) {
            return NULL;
        }
        TAILQ_INIT(&ev_queue);
        LIST_INIT(&wlist);
    }

    struct aui_window *aw = driver->ops->create_window();
    struct aui_widget *widget = (struct aui_widget *)aw;
    widget->type = WIDGET_TYPE_WINDOW;

    LIST_INSERT_HEAD(&wlist, aw, entries);
    widget_init(widget);

    widget->in_ops = &window_ops;
    widget->mapped = 1;

	widget->window = aw;
    aw->draw_flag = 1;
    return aw;
}

void
window_free(struct aui_widget *widget)
{
    struct aui_window *aw = (struct aui_window *)widget;

    LIST_REMOVE(aw, entries);
    driver->ops->delete_window(aw);
}

static void
window_mouse_hover(struct aui_widget *widget, uint16_t x, uint16_t y)
{
    struct aui_window *aw = (struct aui_window *)widget;
    struct aui_container *con = (struct aui_container *)widget;
    struct aui_widget *tohover = NULL;
    struct aui_widget *child;

    TAILQ_FOREACH(child, &widget->queue, entries) {
        if (child->mapped && widget_collides_with_point(child, x, y))
            tohover = child;
    }

    if (tohover != con->focus.hover && con->focus.hover != NULL)
        con->focus.hover->in_ops->mouse_unhover(con->focus.hover);

    con->focus.hover = tohover;

    if (tohover != NULL)
        tohover->in_ops->mouse_hover(tohover, x, y);
}

static void
window_mouse_unhover(struct aui_widget *widget)
{

}

static void 
window_mouse_press(struct aui_widget *widget, uint16_t x, uint16_t y, uint8_t button)
{
    struct aui_container *con = (struct aui_container *)widget;
    struct aui_window *aw = (struct aui_window *)widget;
    struct aui_widget *topress = NULL;
    struct aui_widget *child;

    TAILQ_FOREACH(child, &widget->queue, entries) {
        if (child->mapped && widget_collides_with_point(child, x, y))
            topress = child;
    }

    if (topress != con->focus.press && con->focus.press != NULL)
        con->focus.press->in_ops->mouse_release(con->focus.press, x, y, button);

    con->focus.press = topress;

    if (topress != NULL)
        topress->in_ops->mouse_press(topress, x, y, button);
}

static void 
window_mouse_release(struct aui_widget *widget, uint16_t x, uint16_t y, uint8_t button)
{
    struct aui_container *con = (struct aui_container *)widget;
    struct aui_window *aw = (struct aui_window *)widget;

    if (con->focus.press != NULL) {
        con->focus.press->in_ops->mouse_release(con->focus.press, x, y, button);
        con->focus.press = NULL;

        window_mouse_hover(widget, x, y);
    }
}

static struct aui_geometry
window_get_min_size(struct aui_widget *widget)
{
    struct aui_window *aw = (struct aui_window *)widget;
    struct aui_geometry geom = { 0 };
    geom.width = aw->config.width;
    geom.height = aw->config.height;
    return geom;
}
