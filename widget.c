#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "driver.h"
#include "widget.h"
#include "primitive.h"

void
widget_init(struct aui_widget *widget)
{
    TAILQ_INIT(&(widget)->queue);
    widget->geom = (struct aui_geometry) { 0 };
    widget->mapped = 0;

    memset(&widget->placepar, 0, sizeof(struct aui_placepar));
    memset(&widget->gridpar, 0, sizeof(struct aui_gridpar));
    memset(&widget->packpar, 0, sizeof(struct aui_packpar));
}

void
widget_add(struct aui_widget *parent, struct aui_widget *child)
{
    child->parent = parent;
    child->window = parent->window;
    TAILQ_INSERT_TAIL(&parent->queue, child, entries);
}

void
widget_remove(struct aui_widget *parent, struct aui_widget *child)
{
    TAILQ_REMOVE(&parent->queue, child, entries);
}

void
widget_draw(struct aui_widget *widget)
{
    if (widget->mapped == 0)
        return;

    struct primitive *prim;
    for (int i = 0; i < widget->primitives.count; i++) {
        prim = widget->primitives.list[i];

        switch (prim->type) {
        case PRIMITIVE_TYPE_RECTANGLE:
            driver->ops->render_rectangle(widget->window, prim);
            break;
        case PRIMITIVE_TYPE_TEXT:
            driver->ops->render_text(widget->window, prim);
            break;
        default:
            break;
        }
    }

    struct aui_widget *child;
    TAILQ_FOREACH(child, &widget->queue, entries) {
        widget_draw(child);
    }
}

int
widget_collides_with_point(struct aui_widget *widget, uint16_t x, uint16_t y)
{
    return (x >= widget->geom.x && x < (widget->geom.x + widget->geom.width) &&
            y >= widget->geom.y && y < (widget->geom.y + widget->geom.height));
}
