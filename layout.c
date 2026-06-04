#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
            con->map_count++;

            switch (child->type) {
            case WIDGET_TYPE_FRAME:
                layout_organize(child);
                break;
            default:
                break;
            }
        }
        break;
    case AUI_LAYOUT_GRID: {
        unsigned int cell_size[2] = { 50, 50 };
        unsigned int *added = calloc(con->grid_size[0], sizeof(unsigned int));
        unsigned int *radded = calloc(con->grid_size[1], sizeof(unsigned int));

        TAILQ_FOREACH(child, &widget->queue, entries) {
            if (child->mapped == 0)
                continue;

            struct aui_geometry geom = { 0 };
            struct aui_widget *sub;

            geom.x = widget->geom.x;
            geom.y = widget->geom.y;

            memset(added, 0, sizeof(unsigned int) * con->grid_size[0]);
            memset(radded, 0, sizeof(unsigned int) * con->grid_size[1]);

            TAILQ_FOREACH(sub, &widget->queue, entries) {
                if (sub->mapped == 0 || sub == child)
                    continue;

                if (sub->gridpar.column < child->gridpar.column && added[sub->gridpar.column] == 0) {
                    added[sub->gridpar.column] = 1;
                    geom.x += cell_size[0];
                }

                if (sub->gridpar.row < child->gridpar.row && radded[sub->gridpar.row] == 0) {
                    radded[sub->gridpar.row] = 1;
                    geom.y += cell_size[1];
                }
            }

            geom.width = cell_size[0];
            geom.height = cell_size[1];

            child->in_ops->set_geometry(child, &geom);
        }

        free(added);
        free(radded);

        con->map_count++;

        TAILQ_FOREACH(child, &widget->queue, entries) {
            switch (child->type) {
            case WIDGET_TYPE_FRAME:
                layout_organize(child);
                break;
            default:
                break;
            }
        }
        break;
    }
    case AUI_LAYOUT_PACK: {
        unsigned int cell_size[] = { 50, 50 };
        struct aui_geometry geom = { 0 };
        struct aui_placepar *par = NULL;
        int dx = 0, dy = 0;

        TAILQ_FOREACH(child, &widget->queue, entries) {
            geom.height = cell_size[1];
            geom.width = cell_size[0];
            geom.y = dy;
            geom.x = widget->geom.x + widget->geom.width/2 - geom.width / 2;

            dy += geom.height;
            child->in_ops->set_geometry(child, &geom);
        }

        TAILQ_FOREACH(child, &widget->queue, entries) {
            switch (child->type) {
            case WIDGET_TYPE_FRAME:
                layout_organize(child);
                break;
            default:
                break;
            }
        }
        break;
    }
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
    struct aui_widget *parent;
    struct aui_container *con;

    if (check == -1)
        return - 1;

    con = (struct aui_container *)widget->parent;
    parent = widget->parent;
    con->layout_type = AUI_LAYOUT_PACK;
    widget->packpar = *(par);
    widget->mapped = 1;

    layout_organize(parent);

    return 0;
}

int
aui_grid(struct aui_widget *widget, struct aui_gridpar *par)
{
    int check = can_add(widget, AUI_LAYOUT_GRID);
    struct aui_widget *parent;
    struct aui_container *con;

    if (check == -1)
        return -1;

    con = (struct aui_container *)widget->parent;
    parent = widget->parent;
    con->layout_type = AUI_LAYOUT_GRID;
    widget->gridpar = *(par);
    widget->mapped = 1;

    con->grid_size[0] = (con->grid_size[0] > par->column + 1) ? con->grid_size[0] : par->column + 1;
    con->grid_size[1] = (con->grid_size[1] > par->row + 1) ? con->grid_size[1] : par->row + 1;

    layout_organize(parent);

    return 0;
}

int
aui_getplacepar(struct aui_widget *widget, struct aui_placepar *par)
{
    *par = widget->placepar;
    return 0;
}

int
aui_getgridpar(struct aui_widget *widget, struct aui_gridpar *par)
{
    *par = widget->gridpar;
    return 0;
}
