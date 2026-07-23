#ifndef __DRIVER_H__
#define __DRIVER_H__

#include "aui.h"
#include "widget.h"

enum driver_type {
	DRIVER_TYPE_X11
};

struct aui_dri {
	struct dri_ops *ops;
};

struct dri_ops {
	struct aui_window *(*create_window)(struct aui_window_config*);
	void (*delete_window)(struct aui_window *);
	void (*handle_events)(void);
	void (*resize_window)(struct aui_window *);
	void (*render_background)(struct aui_window *);
	void (*render_foreground)(struct aui_window *);

	struct primitive *(*create_rectangle)(void);
	void (*delete_rectangle) (struct primitive *);
	void (*render_rectangle) (struct aui_window *, struct primitive *);
	void (*set_rectangle_color) (struct aui_window *, struct primitive *, struct aui_color *);
	void (*set_rectangle_geometry) (struct aui_window *, struct primitive *, struct aui_geometry *);

	struct primitive *(*create_text) (void);
	void (*set_text) (struct primitive *, const char *, int16_t, int16_t);
	void (*set_text_color) (struct primitive *, struct aui_color *);
	void (*render_text) (struct aui_window *, struct primitive *);
	struct aui_geometry (*get_text_geometry) (struct primitive *);

	struct aui_canvas *(*create_canvas)();
};

extern struct aui_dri *driver; /* Associated with xcb backend */

int driver_open(enum driver_type);
int xcb_driver_open(void);

#endif
