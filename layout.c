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

    if ((con->layout_type != type) && (con->layout_type != AUI_LAYOUT_NONE)) {
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
    case AUI_LAYOUT_GRID: {
        uint8_t *rmap = calloc(con->grid_size[1], sizeof(uint8_t));
        uint8_t *cmap = calloc(con->grid_size[0], sizeof(uint8_t));

        unsigned int *rsize = calloc(con->grid_size[1], sizeof(unsigned int));
        unsigned int *csize = calloc(con->grid_size[0], sizeof(unsigned int));

        TAILQ_FOREACH(child, &widget->queue, entries) {
            if (child->mapped == 0)
                continue;

            struct aui_geometry min = child->in_ops->get_min_size(child);

            rmap[child->gridpar.row] = 1;
            cmap[child->gridpar.column] = 1;

            rsize[child->gridpar.row] = (rsize[child->gridpar.row] < min.height) ?
                min.height : rsize[child->gridpar.row];

            csize[child->gridpar.column] = (csize[child->gridpar.column] < min.width) ?
                min.width : csize[child->gridpar.column];
        }

        unsigned int *rpos = calloc(con->grid_size[1], sizeof(unsigned int));
        unsigned int *cpos = calloc(con->grid_size[0], sizeof(unsigned int));

        unsigned int rowdt = 0;
        unsigned int coldt = 0;

        for (int r = 0; r < con->grid_size[1]; r++) {
            rpos[r] = rowdt;
            rowdt = (rmap[r] == 1) ? rowdt + rsize[r] : rowdt;
        }

        for (int c = 0; c < con->grid_size[0]; c++) {
            cpos[c] = coldt;
            coldt = (cmap[c] == 1) ? coldt + csize[c] : coldt;
        }

        TAILQ_FOREACH(child, &widget->queue, entries) {
            if (child->mapped == 0)
                continue;

            struct aui_geometry min = child->in_ops->get_min_size(child);
            struct aui_geometry geom = { 0 };

            int dx = (csize[child->gridpar.column] - min.width);
            int dy = (rsize[child->gridpar.row] - min.height);

            geom.x = widget->geom.x + cpos[child->gridpar.column] + dx / 2;
            geom.y = widget->geom.y + rpos[child->gridpar.row] + dy / 2;
            geom.width = min.width;
            geom.height = min.height;

            child->in_ops->set_geometry(child, &geom);
        }

        free(rsize);
        free(csize);
        free(rpos);
        free(cpos);
        free(rmap);
        free(cmap);

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
    /*
     * As far as I'm aware of, Tk's pack is just a rectangle that shrinks
     * depending on the side of the widget being added.
     */
    case AUI_LAYOUT_PACK: {
        struct aui_geometry space = widget->in_ops->get_min_size(widget);

        TAILQ_FOREACH(child, &widget->queue, entries) {
            if (child->mapped == 0)
                continue;

            struct aui_geometry cgeom = child->in_ops->get_min_size(child);

            switch (child->packpar.side) {
                case AUI_SIDE_TOP:
                    cgeom.width = (cgeom.width > space.width) ? space.width : cgeom.width;
                    cgeom.height = (cgeom.height > space.height) ? space.height : cgeom.height;
                    cgeom.x = space.x + space.width / 2 - cgeom.width / 2;
                    cgeom.y = space.y;
                    space.y += cgeom.height;
                    space.height -= cgeom.height;
                    break;
                case AUI_SIDE_LEFT:
                    cgeom.width = (cgeom.width > space.width) ? space.width : cgeom.width;
                    cgeom.height = (cgeom.height > space.height) ? space.height : cgeom.height;
                    cgeom.x = space.x;
                    cgeom.y = space.y + space.height / 2 - cgeom.height / 2;
                    space.width -= cgeom.width;
                    space.x += cgeom.width;
                    break;
                case AUI_SIDE_RIGHT:
                    cgeom.width = (cgeom.width > space.width) ? space.width : cgeom.width;
                    cgeom.height = (cgeom.height > space.height) ? space.height : cgeom.height;
                    cgeom.x = space.x + space.width - cgeom.width;
                    cgeom.y = space.y + space.height / 2 - cgeom.height / 2;
                    space.width -= cgeom.width;
                    break;
                case AUI_SIDE_BOTTOM:
                    cgeom.width = (cgeom.width > space.width) ? space.width : cgeom.width;
                    cgeom.height = (cgeom.height > space.height) ? space.height : cgeom.height;
                    cgeom.x = space.x + space.width / 2 - cgeom.width / 2;
                    cgeom.y = space.y + space.height - cgeom.height;
                    space.height -= cgeom.height;
                    break;
                default:
                    continue;
            }
            child->in_ops->set_geometry(child, &cgeom);
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
aui_getpackpar(struct aui_widget *widget, struct aui_packpar *par)
{
    *par = widget->packpar;
    return 0;
}

int
aui_getgridpar(struct aui_widget *widget, struct aui_gridpar *par)
{
    *par = widget->gridpar;
    return 0;
}
