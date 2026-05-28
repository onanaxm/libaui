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

void
layout_organize(struct aui_widget *widget)
{
    struct aui_container *con = (struct aui_container *)widget;
    struct aui_widget *child;
    int dx, dy;

    switch (con->layout_type) {
    case AUI_LAYOUT_PLACE:
        TAILQ_FOREACH(child, &widget->queue, entries) {
            if (child->mapped == 0)
                continue;

            struct aui_placepar *par = &child->placepar;

            struct aui_geometry geom = { 
                .x = widget->geom.x + child->placepar.x + widget->geom.width * child->placepar.relx,
                .y = widget->geom.y + child->placepar.y + widget->geom.height * child->placepar.rely,
                .width = child->placepar.width + child->placepar.relwidth * widget->geom.width,
                .height = child->placepar.height + child->placepar.relheight * widget->geom.height };

            switch (par->anchor) {
            case AUI_ANCHOR_NW:
                dx = 0;
                dy = 0;
                break;
            case AUI_ANCHOR_NE:
                dx = -geom.width;
                dy = 0;
                break;
            case AUI_ANCHOR_N:
                dx = -geom.width / 2;
                dy = 0;
                break;
            case AUI_ANCHOR_W:
                dx = 0;
                dy = -geom.height / 2;
                break;
            case AUI_ANCHOR_E:
                dx = -geom.width;
                dy = -geom.height /2;
                break;
            case AUI_ANCHOR_SE:
                dx = -geom.width;
                dy = -geom.height;
                break;
            case AUI_ANCHOR_SW:
                dx = 0;
                dy = -geom.height;
                break;
            case AUI_ANCHOR_S:
                dx = -geom.width / 2;
                dy = -geom.height;
                break;
            case AUI_ANCHOR_CENTER:
                dx = -geom.width / 2;
                dy = -geom.height / 2;
                break;
            default:
                break;
            }

            geom.x += dx;
            geom.y += dy;
            child->in_ops->set_geometry(child, &geom); 

            switch (child->type) {
            case WIDGET_TYPE_FRAME:
                layout_organize(child);
                break;
            default:
                break;
            }
        }
        break;
    default:
        break;
    }
}

int 
aui_place(struct aui_widget *widget, struct aui_placepar *par)
{
    int check = can_add(widget, AUI_LAYOUT_PLACE);
    struct aui_widget *parent;
    struct aui_container *con;

    if (check == -1)
        return -1;

    con = (struct aui_container *)widget->parent;
    parent = (struct aui_widget *)widget->parent;
    con->layout_type = AUI_LAYOUT_PLACE;
    widget->placepar = *(par);
    widget->mapped = 1;

    layout_organize(parent);

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

int
aui_getplacepar(struct aui_widget *widget, struct aui_placepar *par)
{
    *par = widget->placepar;
    return 0;
}
