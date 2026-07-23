#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aui.h"
#include "widget.h"

static struct aui_placepar default_place = {
	.x = 0,
	.y = 0,
	.relx = 0,
	.rely = 0,
	.width = 0,
	.height = 0,
	.relwidth = 0,
	.relheight = 0,
	.anchor = AUI_ANCHOR_NW,
};

static struct aui_packpar default_pack = {
	.anchor = AUI_ANCHOR_NW,
	.fill = AUI_FILL_NONE,
	.side = AUI_SIDE_TOP,
	.expand = 0,
};

static struct aui_gridpar default_grid = {
	.column = 0,
	.row = 0,
	.columnspan = 1,
	.rowspan = 1,
	.ipadx = 0,
	.ipady = 0,
	.padx = 0,
	.pady = 0
};

static struct aui_rowpar default_rowconfigure = {
	.minsize = 0,
	.weight = 0,
	.uniform = 0,
	.pad = 0,
};

static struct aui_columnpar default_columnconfigure = {
	.minsize = 0,
	.weight = 0,
	.uniform = 0,
	.pad = 0,
};

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
place(struct aui_widget *widget)
{
	struct aui_widget *child;

	TAILQ_FOREACH(child, &widget->queue, entries) {
		if (child->mapped == 0)
			continue;

		struct aui_placepar *par = &child->placepar;
		int dx = 0, dy = 0;

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
		child->ops->set_geometry(child, &geom); 

		switch (child->type) {
		case WIDGET_TYPE_FRAME:
			layout_organize(child);
			break;
		default:
			break;
		}
	}
}

void
grid(struct aui_widget *widget)
{
	struct aui_container *con = (struct aui_container *)widget;
	uint8_t *rmap = con->row_maps;
	uint8_t *cmap = con->col_maps;
	struct aui_widget *child;

	unsigned int *rsize = con->row_sizes;
	unsigned int *csize = con->col_sizes;

	memset(con->row_maps, 0, sizeof(*con->row_maps) * con->grid_size[1]);
	memset(con->col_maps, 0, sizeof(*con->col_maps) * con->grid_size[0]);

	memset(con->row_sizes, 0, sizeof(*con->row_sizes) * con->grid_size[1]);
	memset(con->col_sizes, 0, sizeof(*con->col_sizes) * con->grid_size[0]);

	TAILQ_FOREACH(child, &widget->queue, entries) {
		if (child->mapped == 0)
			continue;

		struct aui_geometry min = child->ops->get_min_size(child);

		rmap[child->gridpar.row] = 1;
		cmap[child->gridpar.column] = 1;

		rsize[child->gridpar.row] = (rsize[child->gridpar.row] < min.height) ?
			min.height : rsize[child->gridpar.row];

		csize[child->gridpar.column] = (csize[child->gridpar.column] < min.width) ?
			min.width : csize[child->gridpar.column];
	}

	unsigned int *rpos = con->row_pos;
	unsigned int *cpos = con->col_pos;

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

		struct aui_geometry min = child->ops->get_min_size(child);
		struct aui_geometry geom = { 0 };
		struct aui_gridpar *gpar = &child->gridpar;

		int cspan = (gpar->column + gpar->columnspan) <= con->grid_size[0] ?
					gpar->columnspan - 1: con->grid_size[0] - gpar->column - 1;

		int rspan = (gpar->row + gpar->rowspan) <= con->grid_size[1] ?
					gpar->rowspan - 1: con->grid_size[1] - gpar->row - 1;


		int dx = (csize[child->gridpar.column] - min.width);
		int dy = (rsize[child->gridpar.row] - min.height);

		int width = min.width;
		int height = min.height;

		for (int i = gpar->column; i < gpar->column + cspan; i++)
			width += csize[i];

		for (int i = gpar->row; i < gpar->row + rspan; i++)
			height += rsize[i];

		geom.x = widget->geom.x + cpos[child->gridpar.column] + dx / 2;
		geom.y = widget->geom.y + rpos[child->gridpar.row] + dy / 2;
		geom.width = width;
		geom.height = height;

		child->ops->set_geometry(child, &geom);
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
}

void
pack(struct aui_widget *widget)
{
	struct aui_container *con = (struct aui_container *)widget;
	struct aui_widget *child;
	struct aui_geometry space = widget->geom;
	unsigned int hexpand = 0;
	unsigned int vexpand = 0;
	unsigned int req_w = 0;
	unsigned int req_h = 0;
	int extra_w = 0;
	int extra_h = 0;
	int hextra = 0;
	int vextra = 0;

	/*
	 * Compute requested and extra space. hextra and vextra are only set
	 * if there is no top/bottom widgets succeeding it. This guarantees
	 * a proper request size.
	 */
	TAILQ_FOREACH(child, &widget->queue, entries) {
		if (child->mapped == 0)
			continue;

		struct aui_geometry cgeom = child->ops->get_min_size(child);
		switch (child->packpar.side) {
		case AUI_SIDE_TOP:
		case AUI_SIDE_BOTTOM:
			vexpand = (child->packpar.expand) ? vexpand + 1 : vexpand;
			if (vextra) vextra = 0;
			req_h += cgeom.height;
			hextra = (cgeom.width > hextra) ? cgeom.width : hextra;
			break;
		case AUI_SIDE_LEFT:
		case AUI_SIDE_RIGHT:
			hexpand = (child->packpar.expand) ? hexpand + 1 : hexpand;
			if (hextra) hextra = 0;
			vextra = (cgeom.height > vextra) ? cgeom.height : vextra;
			req_w += cgeom.width;
			break;
		default:
			break;
		}
	}

	req_w += hextra;
	req_h += vextra;

	if (req_w > space.width)
		req_w = space.width;

	if (req_h > space.height)
		req_h = space.height;

	extra_w = space.width - req_w;
	extra_h = space.height - req_h;

	/*
	 * Making sure each widget consume space before expension
	 * for their area.
	 */
	TAILQ_FOREACH(child, &widget->queue, entries) {
		if (child->mapped == 0)
			continue;

		struct aui_geometry min = child->ops->get_min_size(child);

		switch (child->packpar.side) {
		case AUI_SIDE_TOP:
			child->packarea = (struct aui_geometry) {
				.x = space.x,
				.y = space.y,
				.width = space.width,
				.height = min.height,
			};

			if (child->packarea.height > space.height) {
				child->packarea.height = space.height;
			}

			space.y += child->packarea.height;
			space.height -= child->packarea.height;
			break;
		case AUI_SIDE_BOTTOM:
			child->packarea = (struct aui_geometry) {
				.x = space.x,
				.y = space.y,
				.width = space.width,
				.height = min.height
			};

			if (child->packarea.height > space.height) {
				child->packarea.height = space.height;
			}

			child->packarea.y = space.y + space.height - child->packarea.height;

			space.height -= child->packarea.height;
			break;
		case AUI_SIDE_LEFT:
			child->packarea = (struct aui_geometry) {
				.x = space.x,
				.y = space.y,
				.width = min.width,
				.height = space.height
			};

			if (child->packarea.width > space.width) {
				child->packarea.width = space.width;
			}

			space.x += child->packarea.width;
			space.width -= child->packarea.width;
			break;
		case AUI_SIDE_RIGHT:
			child->packarea = (struct aui_geometry) {
				.x = space.x,
				.y = space.y,
				.width = min.width,
				.height = space.height
			};

			if (child->packarea.width > space.width) {
				child->packarea.width = space.width;
			}

			child->packarea.x = space.x + space.width - child->packarea.width;

			space.width -= child->packarea.width;
			break;
		default:
			break;
		}
	}

	int top = 0;
	int bottom = 0;
	int right = 0;
	int left = 0;

	/*
	 * Consuming the remaining space if expanded
	 */
	TAILQ_FOREACH(child, &widget->queue, entries) {
		if (child->mapped == 0)
			continue;

		if (space.width == 0 && space.height == 0)
			break;

		struct aui_geometry *area = &child->packarea;

		switch (child->packpar.side) {
		case AUI_SIDE_TOP:
			area->x += left;
			area->y += top;
			area->width -= left;

			area->width -= right;

			if (child->packpar.expand == 1) {
				if (vexpand) {
					top += extra_h / vexpand;
					area->height += extra_h / vexpand;
					extra_h -= extra_h / vexpand;
					vexpand--;
				}
			}
			break;

		case AUI_SIDE_BOTTOM:
			area->x += left;
			area->width -= left;

			area->width -= right;
			if (child->packpar.expand == 1) {
				if (vexpand) {
					bottom += extra_h / vexpand;
					area->height += extra_h / vexpand;
					area->y -= extra_h / vexpand;
					extra_h -= extra_h / vexpand;
					vexpand--;
				}
			}
			break;
		case AUI_SIDE_LEFT:
			area->y += top;
			area->x += left;
			area->height -= top;
			area->height -= bottom;

			if (child->packpar.expand == 1) {
				if (hexpand) {
					left += extra_w / hexpand;
					area->width += extra_w / hexpand;
					extra_w -= extra_w / hexpand;
					hexpand--;
				}
			}
			break;
		case AUI_SIDE_RIGHT:
			area->y += top;
			area->height -= top;
			area->height -= bottom;

			if (child->packpar.expand == 1) {
				if (hexpand) {
					right += extra_w / hexpand;
					area->x -= extra_w / hexpand;
					area->width += extra_w / hexpand;
					extra_w -= extra_w / hexpand;
					hexpand--;
				}
			}
			break;
		default:
			break;
		}
	}

	/*
	 * Setting geometry
	 */
	TAILQ_FOREACH(child, &widget->queue, entries) {
		if (child->mapped == 0)
			continue;

		struct aui_geometry min = child->ops->get_min_size(child);
		struct aui_geometry *area = &child->packarea;

		switch (child->packpar.fill) {
		case AUI_FILL_NONE:
			child->geom.width = (min.width > area->width) ?  area->width : min.width;
			child->geom.height = (min.height > area->height) ?  area->height : min.height;
			break;
		case AUI_FILL_X:
			child->geom.width = area->width;
			child->geom.height = (min.height > area->height) ?  area->height : min.height;
			break;
		case AUI_FILL_Y:
			child->geom.width = (min.width > area->width) ?  area->width : min.width;
			child->geom.height = area->height;
			break;
		case AUI_FILL_BOTH:
			child->geom.width = area->width;
			child->geom.height = area->height;
			break;
		default:
			break;
		}

		switch (child->packpar.anchor) {
		case AUI_ANCHOR_NW:
			child->geom.x = area->x;
			child->geom.y = area->y;
			break;
		case AUI_ANCHOR_N:
			child->geom.x = area->x + area->width / 2 - child->geom.width / 2;
			child->geom.y = area->y;
			break;
		case AUI_ANCHOR_NE:
			child->geom.x = area->x + area->width - child->geom.width;
			child->geom.y = area->y;
			break;
		case AUI_ANCHOR_CENTER:
			child->geom.x = area->x + area->width / 2 - child->geom.width / 2;
			child->geom.y = area->y + area->height / 2 - child->geom.height / 2;
			break;
		case AUI_ANCHOR_SE:
			child->geom.x = area->x + area->width - child->geom.width;
			child->geom.y = area->y + area->height - child->geom.height;
			break;
		case AUI_ANCHOR_S:
			child->geom.x = area->x + area->width / 2 - child->geom.width / 2;
			child->geom.y = area->y + area->height - child->geom.height;
			break;
		case AUI_ANCHOR_SW:
			child->geom.x = area->x;
			child->geom.y = area->y + area->height - child->geom.height;
			break;	
		case AUI_ANCHOR_W:
			child->geom.x = area->x;
			child->geom.y = area->y + area->height / 2 - child->geom.height / 2;
			break;
		case AUI_ANCHOR_E:
			child->geom.x = area->x + area->width - child->geom.width;
			child->geom.y = area->y + area->height / 2 - child->geom.height / 2;
			break;
		default:
			break;
		break;
		}

		child->ops->set_geometry(child, &child->geom);
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
}

void
layout_organize(struct aui_widget *widget)
{
	struct aui_container *con = (struct aui_container *)widget;
	struct aui_widget *child;

	switch (con->layout_type) {
	case AUI_LAYOUT_PLACE:
		place(widget);
		break;
	case AUI_LAYOUT_GRID: 
		grid(widget);
		break;
	/*
	 * Lots of suffering into implementing this one :D
	 */
	case AUI_LAYOUT_PACK:
		pack(widget);
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

	if (par == NULL)
		par = &default_place;

	con = (struct aui_container *)widget->parent;
	parent = (struct aui_widget *)widget->parent;
	con->layout_type = AUI_LAYOUT_PLACE;
	widget->placepar = *(par);
	widget->mapped = 1;
	con->map_count++;

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

	if (par == NULL)
		par = &default_pack;

	con = (struct aui_container *)widget->parent;
	parent = widget->parent;
	con->layout_type = AUI_LAYOUT_PACK;
	widget->packpar = *(par);
	widget->mapped = 1;
	con->map_count++;

	/*
	 * This is important for packing order
	 */
	widget_lead(widget->parent, widget);

	layout_organize(parent);

	return 0;
}

int
aui_grid(struct aui_widget *widget, struct aui_gridpar *par)
{
	int check = can_add(widget, AUI_LAYOUT_GRID);
	struct aui_widget *parent;
	struct aui_container *con;
	unsigned int *rsizes;
	unsigned int *csizes;
	unsigned int *rpos;
	unsigned int *cpos;
	uint8_t *rmaps;
	uint8_t *cmaps;

	if (check == -1)
		return -1;

	if (par == NULL)
		par = &default_grid;

	par->columnspan = (par->columnspan < 1) ? 1 : par->columnspan;
	par->rowspan = (par->rowspan < 1) ? 1 : par->rowspan;
	con = (struct aui_container *)widget->parent;
	parent = widget->parent;
	con->layout_type = AUI_LAYOUT_GRID;
	widget->gridpar = *(par);
	widget->mapped = 1;
	con->map_count++;

	con->grid_size[0] = (con->grid_size[0] > (par->column + par->columnspan)) ? 
		con->grid_size[0] : (par->column + par->columnspan);

	con->grid_size[1] = (con->grid_size[1] > (par->row + par->rowspan)) ? 
		con->grid_size[1] : (par->row + par->rowspan);

	rmaps = reallocarray(con->row_maps, con->grid_size[1], sizeof(*con->row_maps));
	if (rmaps == NULL) {
		fprintf(stderr, "libaui: failed to allocate row maps\n");
		return -1;
	}
	con->row_maps = rmaps;

	cmaps = reallocarray(con->col_maps, con->grid_size[0], sizeof(*con->col_maps));
	if (cmaps == NULL) {
		fprintf(stderr, "libaui: failed to allocate col maps\n");
		return -1;
	}
	con->col_maps = cmaps;


	rsizes = reallocarray(con->row_sizes, con->grid_size[1], sizeof(*con->row_sizes));
	if (rsizes == NULL) {
		fprintf(stderr, "libaui: failed to allocated row sizes\n");
		return -1;
	}
	con->row_sizes = rsizes;

	csizes = reallocarray(con->col_sizes, con->grid_size[0], sizeof(*con->col_sizes));
	if (csizes == NULL) {
		fprintf(stderr, "libaui: failed to allocated col sizes\n");
		return -1;
	}
	con->col_sizes = csizes;

	rpos = reallocarray(con->row_pos, con->grid_size[1], sizeof(*con->row_pos));
	if (rpos == NULL) {
		fprintf(stderr, "libaui: failed to allocate row positions\n");
		return -1;
	}
	con->row_pos = rpos;

	cpos = reallocarray(con->col_pos, con->grid_size[0], sizeof(*con->col_pos));
	if (cpos == NULL) {
		fprintf(stderr, "libaui: failed to allocate col positions\n");
		return -1;
	}
	con->col_pos = cpos;

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
