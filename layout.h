#ifndef LAYOUT_H
#define LAYOUT_H

enum aui_layout_type {
	AUI_LAYOUT_NONE,
	AUI_LAYOUT_PLACE,
	AUI_LAYOUT_GRID,
	AUI_LAYOUT_PACK
};

void layout_organize(struct aui_widget *widget);

#endif
