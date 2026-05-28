#include <stdio.h>
#include <stdlib.h>

#include "aui.h"
#include "widget.h"

static int
can_add(struct aui_widget *widget, enum aui_layout_type type)
{
    struct aui_widget *parent = widget->parent;

    if (parent->type != WIDGET_TYPE_WINDOW && parent->type != WIDGET_TYPE_CANVAS &&
        parent->type != WIDGET_TYPE_FRAME) {
        fprintf(stderr, "libaui: cannot map widget in non-container\n");
        return -1;
    }

    struct aui_container *con = (struct aui_container *)parent;

    if ((con->layout_type != type) && (con->layout_type != AUI_LAYOUT_NONE && con->map_count != 0)) {
        fprintf(stderr, "libaui: cannot map as mismatching type / non-empty container\n");
        return -1;
    }

    return 0;
}

int 
aui_place(struct aui_widget *widget, struct aui_placepar *par)
{
    int check = can_add(widget, AUI_LAYOUT_PLACE);

    if (check == -1)
        return -1;

    struct aui_container *con = (struct aui_container *)widget->parent;
    struct aui_geometry geom = { .x = par->x, .y = par->y, 
                                 .width = par->width, 
                                 .height = par->height };

    widget->geom = geom;

    if (widget->in_ops->set_geometry) {
        widget->in_ops->set_geometry(widget, &geom);
        widget->window->draw_flag = 1;
        widget->mapped = 1;

        con->map_count++;
        con->layout_type = AUI_LAYOUT_PLACE;
    }
    return 0;
}

int
aui_pack(struct aui_widget *widget, struct aui_packpar *par)
{
    int check = can_add(widget, AUI_LAYOUT_PACK);

    if (check == -1)
        return - 1;

    return 0;
}

int
aui_grid(struct aui_widget *widget, struct aui_gridpar *par)
{
    int check = can_add(widget, AUI_LAYOUT_GRID);

    if (check == -1)
        return -1;

    return 0;
}
