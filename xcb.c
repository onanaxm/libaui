#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/render.h>

#include "font.h"
#include "event.h"
#include "driver.h"
#include "widget.h"
#include "primitive.h"

struct aui_dri_xcb {
	struct aui_dri adr;

	struct pollfd pollfd; /* One reason, POSIX */

	xcb_connection_t *conn;
	xcb_screen_t *screen;
	xcb_intern_atom_reply_t *wm_protocols;
	xcb_intern_atom_reply_t *wm_delete_window;

	xcb_render_glyphset_t glyphset;
	struct glyph *glyphs;
	int font_height;
	xcb_render_pictformat_t fmt_normal;
	xcb_render_pictformat_t fmt_alpha8;
	xcb_render_pictformat_t fmt_argb32;
};

struct aui_window_xcb {
	struct aui_window aw;

	xcb_window_t xw;
	xcb_pixmap_t pixmap;
	xcb_render_picture_t dst_pict;
	xcb_render_picture_t src_pict;
};

struct aui_dri *driver;

static struct aui_window *aui_dri_create_window(struct aui_window_config *);
static void aui_dri_delete_window(struct aui_window *);
static void aui_dri_handle_events(void);
static void aui_dri_resize_window(struct aui_window *);
static void aui_dri_render_background(struct aui_window *);
static void aui_dri_render_foreground(struct aui_window *);

static struct primitive *aui_dri_create_rectangle(void);
static void aui_dri_delete_rectangle(struct primitive *);
static void aui_dri_render_rectangle(struct aui_window *, struct primitive *);
static void aui_dri_set_rectangle_color(struct aui_window *, struct primitive *, struct aui_color *);
static void aui_dri_set_rectangle_geometry(struct aui_window *, struct primitive *, 
	struct aui_geometry *);

static struct primitive *aui_dri_create_text(void);
static void aui_dri_set_text(struct primitive *, const char *, int16_t, int16_t);
static void aui_dri_set_text_color(struct primitive*, struct aui_color*);
static void aui_dri_render_text(struct aui_window *, struct primitive *);
static struct aui_geometry aui_dri_get_text_geometry(struct primitive *);

struct aui_dri_canvas {
	struct aui_canvas can;
};

static struct dri_ops aui_dri_ops = {
	aui_dri_create_window,
	aui_dri_delete_window,
	aui_dri_handle_events,
	aui_dri_resize_window,
	aui_dri_render_background,
	aui_dri_render_foreground,

	aui_dri_create_rectangle,
	aui_dri_delete_rectangle,
	aui_dri_render_rectangle,
	aui_dri_set_rectangle_color,
	aui_dri_set_rectangle_geometry,

	aui_dri_create_text,
	aui_dri_set_text,
	aui_dri_set_text_color,
	aui_dri_render_text,
	aui_dri_get_text_geometry
};

struct rectangle {
	struct primitive base;
	xcb_rectangle_t xrect;
	xcb_render_color_t xcolor;
};

struct text {
	struct primitive base;
	const char *data; /* Not allocated, pointer to string literal */
	xcb_render_color_t xcolor;
	xcb_render_picture_t xpict;
	int16_t x, y;
};

static int config_xcb(struct aui_dri_xcb*);
static void set_pixmap_xcb(struct aui_window_xcb *);
static struct aui_window_xcb *find_window_xcb(xcb_window_t);
static void set_name_xcb(struct aui_window_xcb *, const char *);

int
xcb_driver_open(void)
{
	struct aui_dri_xcb *dri_xcb;

	dri_xcb = malloc(sizeof(struct aui_dri_xcb));
	if (dri_xcb == NULL)
		return -1;

	/* Setting up the driver */
	dri_xcb->adr.ops = &aui_dri_ops;
	driver = &dri_xcb->adr;

	if (config_xcb(dri_xcb) == -1)
		return -1;


	dri_xcb->font_height = 0;
	dri_xcb->glyphs = calloc(126, sizeof(struct glyph));
	for (int i = 32; i < 127; i++) dri_xcb->glyphs[i].charcode = i;

	font_init();
	font_load_glyphs(dri_xcb->glyphs + 32, 127 - 32);

	dri_xcb->glyphset = xcb_generate_id(dri_xcb->conn);
	xcb_render_create_glyph_set(dri_xcb->conn, dri_xcb->glyphset, dri_xcb->fmt_alpha8);

	for (int i = 0; i < (127 - 32); i++) {
		uint32_t glyphid = 32 + i;
		struct glyph *g = &dri_xcb->glyphs[i + 32];
		xcb_render_glyphinfo_t ginfo = {
			.width = g->width,
			.height = g->height,
			.x = g->x,
			.y = g->y,
			.x_off = g->x_offset,
			.y_off = 0, /* We are only rendering horizontaly. This is unecessary */
		};

		xcb_render_add_glyphs(
			dri_xcb->conn,
			dri_xcb->glyphset,
			1,
			&glyphid,
			&ginfo,
			g->stride * g->height,
			g->bitmap
		);

		if (dri_xcb->font_height < g->y_offset)
			dri_xcb->font_height = g->y_offset;
	}

	return 0;
}

static struct aui_window
*aui_dri_create_window(struct aui_window_config *config)
{
	struct aui_window_xcb *aw_xcb;
	struct aui_dri_xcb *dri_xcb;
	struct aui_window *aw;

	aw_xcb = calloc(1, sizeof(struct aui_window_xcb));

	if (aw_xcb == NULL)
		return NULL;

	dri_xcb = (struct aui_dri_xcb*)driver;
	aw = &aw_xcb->aw;


	int mask = XCB_CW_BACK_PIXMAP | XCB_CW_EVENT_MASK;
	uint32_t values[2] = {XCB_NONE,
		XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY |
		XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
		XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_PRESS|
		XCB_EVENT_MASK_BUTTON_RELEASE};

	aw_xcb->xw = xcb_generate_id(dri_xcb->conn);
	xcb_create_window(dri_xcb->conn,
					  XCB_COPY_FROM_PARENT,
					  aw_xcb->xw,
					  dri_xcb->screen->root,
					  0, 0,
					  config->width, config->height,
					  0,
					  XCB_WINDOW_CLASS_INPUT_OUTPUT,
					  dri_xcb->screen->root_visual,
					  mask, values);

	xcb_atom_t data[] = { dri_xcb->wm_delete_window->atom };
	xcb_change_property(dri_xcb->conn,
						XCB_PROP_MODE_REPLACE,
						aw_xcb->xw,
						dri_xcb->wm_protocols->atom,
						XCB_ATOM_ATOM, 
						32, 
						1,
						(const void *)data);

	struct aui_widget *ww = (struct aui_widget *)aw;
	ww->geom.width = config->width;
	ww->geom.height = config->height;
	set_pixmap_xcb(aw_xcb);
	set_name_xcb(aw_xcb, config->title);
	xcb_map_window(dri_xcb->conn, aw_xcb->xw);
	xcb_flush(dri_xcb->conn);
	return aw;
}

static void
aui_dri_delete_window(struct aui_window *aw)
{
	struct aui_window_xcb *aw_xcb;
	struct aui_dri_xcb *dri_xcb;

	dri_xcb = (struct aui_dri_xcb *)driver;
	aw_xcb = (struct aui_window_xcb *)aw;

	xcb_destroy_window(dri_xcb->conn, aw_xcb->xw);
	free(aw_xcb);
	xcb_flush(dri_xcb->conn);
}

/*
 * Formats event sent by xcb into aui events
 */
static void
aui_dri_handle_events()
{
	struct aui_dri_xcb *dri_xcb;
	xcb_generic_event_t *event;
	struct aui_window_xcb *aw_xcb;
	struct aui_window *aw;
	struct aui_widget *ww;
	xcb_expose_event_t *exe;
	xcb_motion_notify_event_t *mne;
	xcb_client_message_event_t *cme;
	xcb_configure_notify_event_t *cne;
	xcb_button_press_event_t *bpe;
	xcb_button_release_event_t *bre;

	dri_xcb = (struct aui_dri_xcb *)driver;

	int ret = poll(&dri_xcb->pollfd, 1, 10);

	if (ret < 0)
		return;

	while ((event = xcb_poll_for_event(dri_xcb->conn)) != NULL) {
		switch (event->response_type & ~0x80) {
		case XCB_EXPOSE:
			exe = (xcb_expose_event_t *)event;
			aw_xcb = find_window_xcb(exe->window);
			aw = (struct aui_window *)aw_xcb;
			aw->draw_flag = 1;
			break;
		case XCB_CLIENT_MESSAGE:
			cme = (xcb_client_message_event_t *)event;
			aw_xcb = find_window_xcb(cme->window);
			aw = (struct aui_window *)aw_xcb;

			if (cme->data.data32[0] == dri_xcb->wm_delete_window->atom) {
				struct aui_event_quit qev = {
					.event = { .type = AUI_EVENT_QUIT, .aw = aw },
				};
				aui_add_event(&qev.event);
			}
			break;
		case XCB_CONFIGURE_NOTIFY:
			cne = (xcb_configure_notify_event_t *)event;
			aw_xcb = find_window_xcb(cne->window);
			aw = (struct aui_window *)aw_xcb;
			ww = (struct aui_widget *)aw;

			if (ww->geom.width != cne->width || ww->geom.height != cne->height) {
				struct aui_event_resize rse = {
					.event = { .type = AUI_EVENT_RESIZE, .aw = aw },
					.old_w = ww->geom.width,
					.old_h = ww->geom.height,
					.new_w = cne->width,
					.new_h = cne->height,
				};
				ww->geom.width = cne->width;
				ww->geom.height = cne->height;
				aui_add_event(&rse.event);
				aw->draw_flag = 1;
			}
			break;
		case XCB_MOTION_NOTIFY:
			mne = (xcb_motion_notify_event_t *)event;
			aw_xcb = find_window_xcb(mne->event);
			aw = (struct aui_window *)aw_xcb;

			struct aui_event_mouse_motion mev = {
				.event = { .type = AUI_EVENT_MOUSE_MOTION, .aw = aw },
				.x = mne->event_x, .y = mne->event_y
			};
			aui_add_event(&mev.event);
			break;
		case XCB_BUTTON_PRESS:
			bpe = (xcb_button_press_event_t *)event;
			aw_xcb = find_window_xcb(bpe->event);
			aw = (struct aui_window *)aw_xcb;

			struct aui_event_mouse_press mpe = {
				.event = { .type = AUI_EVENT_MOUSE_PRESS, .aw = aw },
				.x = bpe->event_x, .y = bpe->event_y, .button = bpe->detail
			};
			aui_add_event(&mpe.event);
			break;
		case XCB_BUTTON_RELEASE:
			bre = (xcb_button_release_event_t *)event;
			aw_xcb = find_window_xcb(bre->event);
			aw = (struct aui_window *)aw_xcb;

			struct aui_event_mouse_release mre = {
				.event = { .type = AUI_EVENT_MOUSE_RELEASE, .aw = aw },
				.x = bre->event_x, .y = bre->event_y, .button = bre->detail
			};
			aui_add_event(&mre.event);
			break;
		default:
			break;
		}
		free(event);
	}
}

static void 
aui_dri_resize_window(struct aui_window *aw)
{
	set_pixmap_xcb((struct aui_window_xcb *)aw);
	aw->draw_flag = 1;
}

static void 
aui_dri_render_background(struct aui_window *aw)
{
	xcb_render_color_t color = { .red = 0xd9d9, .green = 0xd9d9, .blue = 0xd9d9, .alpha = 0xffff };
	struct aui_widget *ww = (struct aui_widget *)aw;
	xcb_rectangle_t rects[] = { { .x = 0, .y = 0, .width = ww->geom.width, 
		.height = ww->geom.height } };
	struct aui_dri_xcb *dri_xcb = (struct aui_dri_xcb *)driver;
	struct aui_window_xcb *aw_xcb = (struct aui_window_xcb *)aw;

	xcb_render_fill_rectangles(dri_xcb->conn,
							   XCB_RENDER_PICT_OP_OVER,
							   aw_xcb->src_pict,
							   color,
							   1,
							   rects);
}

static void
aui_dri_render_foreground(struct aui_window *aw)
{
	struct aui_window_xcb *aw_xcb = (struct aui_window_xcb *)aw;
	struct aui_dri_xcb *dri_xcb = (struct aui_dri_xcb *)driver;
	struct aui_widget *ww = (struct aui_widget *)aw;

	xcb_render_composite(dri_xcb->conn,
						 XCB_RENDER_PICT_OP_OVER,
						 aw_xcb->src_pict,
						 XCB_NONE,
						 aw_xcb->dst_pict,
						 0, 0, 0, 0, 0, 0,
						 ww->geom.width, ww->geom.height);

	xcb_flush(dri_xcb->conn);
}

static struct primitive*
aui_dri_create_rectangle()
{
	struct rectangle *rect = calloc(1, sizeof(struct rectangle));

	rect->base.type = PRIMITIVE_TYPE_RECTANGLE;
	rect->xrect.x = 0;
	rect->xrect.y = 0;
	rect->xrect.width = 0;
	rect->xrect.height = 0;

	rect->xcolor.red = 0;
	rect->xcolor.green = 0;
	rect->xcolor.blue = 0;
	rect->xcolor.alpha = 0;

	return &(rect)->base;
}

static void 
aui_dri_delete_rectangle(struct primitive *prim)
{
	struct rectangle *rect = (struct rectangle *)prim;
	free(rect);
}

static void
aui_dri_render_rectangle(struct aui_window *aw, struct primitive *prim)
{
	struct aui_window_xcb *aw_xcb = (struct aui_window_xcb *)aw;
	struct aui_dri_xcb *dri_xcb = (struct aui_dri_xcb *)driver;
	struct rectangle *rect = (struct rectangle *)prim;

	if (prim->type != PRIMITIVE_TYPE_RECTANGLE) {
		fprintf(stderr, "libaui: attempt to render rectangle with mismatching type.\n");
		return;
	}

	xcb_render_fill_rectangles(dri_xcb->conn,
							   XCB_RENDER_PICT_OP_OVER,
							   aw_xcb->src_pict,
							   rect->xcolor,
							   1,
							   &rect->xrect);
}

static void 
aui_dri_set_rectangle_color(struct aui_window *aw, struct primitive *prim, struct aui_color *color)
{
	struct rectangle *rect = (struct rectangle *)prim;

	if (prim->type != PRIMITIVE_TYPE_RECTANGLE) {
		fprintf(stderr, "libaui: attempt to set_rectangle_color for mismatching type\n");
		return;
	}

	rect->xcolor.red = (color->red * 0xFFFF) / 0xFF;
	rect->xcolor.green = (color->green * 0xFFFF) / 0xFF;
	rect->xcolor.blue = (color->blue * 0xFFFF) / 0xFF;
	rect->xcolor.alpha = (color->alpha * 0xFFFF) / 0xFF;

	aw->draw_flag = 1;
}

static 
void aui_dri_set_rectangle_geometry(struct aui_window *aw, struct primitive *prim, 
	struct aui_geometry *geom)
{
	struct rectangle *rect = (struct rectangle *)prim;

	rect->xrect.x = geom->x;
	rect->xrect.y = geom->y;
	rect->xrect.width = geom->width;
	rect->xrect.height = geom->height;
}

static struct primitive*
aui_dri_create_text()
{
	struct aui_dri_xcb *dri_xcb = (struct aui_dri_xcb *)driver;
	struct text *t = calloc(1, sizeof(struct text));

	t->xpict = xcb_generate_id(dri_xcb->conn);
	t->xcolor = (xcb_render_color_t) { 0xffff/2, 0xffff/2, 0, 0xffff };
	xcb_render_create_solid_fill(dri_xcb->conn, t->xpict, t->xcolor);
	return &(t)->base;
}

static void
aui_dri_set_text(struct primitive *p, const char *data, int16_t x, int16_t y)
{
	struct text *t = (struct text *)p;
	t->base.type = PRIMITIVE_TYPE_TEXT;
	t->data = data;
	t->x = x;
	t->y = y;
}

static void
aui_dri_set_text_color(struct primitive *p, struct aui_color *c)
{
	struct aui_dri_xcb *dri_xcb = (struct aui_dri_xcb *)driver;
	struct text *t = (struct text *)p;

	t->xcolor = (xcb_render_color_t) { 
		c->red * 0xffff / 0xff,
		c->green * 0xffff / 0xff,
		c->blue * 0xffff / 0xff,
		c->alpha * 0xffff / 0xff
	};

	xcb_render_free_picture(dri_xcb->conn, t->xpict);
	t->xpict = xcb_generate_id(dri_xcb->conn);
	xcb_render_create_solid_fill(dri_xcb->conn, t->xpict, t->xcolor);
}

/*
 * This renders text without the use of xcb_render utils. It utilises the
 * xcb_render_composite_glyphs_8 function which expects an 8 bytes header
 * where. Can only be up to 255 characters:
 *
 * byte0=>	 len
 * byte1-3=>   padding
 * byte4-5=>   dx
 * byte6-7=>   dy
 */
struct ghead {
	uint8_t	   len;
	uint8_t	   pad[3];
	int16_t	   dx;
	int16_t	   dy;
};

static void 
aui_dri_render_text(struct aui_window *aw, struct primitive *p)
{
	struct aui_window_xcb *aw_xcb = (struct aui_window_xcb *)aw;
	struct aui_dri_xcb *dri_xcb = (struct aui_dri_xcb *)driver;
	struct text *t = (struct text *)p;

	size_t len = (t->data == NULL) ? 0 : strlen(t->data);
	struct ghead cmd = { .len = len, .dx = t->x, .dy = t->y };

	size_t cmd_len = len + 8;
	uint8_t *glyphcmds = malloc(cmd_len);

	if (glyphcmds == NULL)
		return;

	glyphcmds[0] = cmd.len;
	glyphcmds[1] = 0;
	glyphcmds[2] = 0;
	glyphcmds[3] = 0;

	memcpy(&glyphcmds[4], &cmd.dx, 2);
	memcpy(&glyphcmds[6], &cmd.dy, 2);

	memcpy(glyphcmds + 8, t->data, len);

	xcb_render_composite_glyphs_8(
		dri_xcb->conn,
		XCB_RENDER_PICT_OP_OVER,
		t->xpict,
		aw_xcb->src_pict,
		0,
		dri_xcb->glyphset,
		0, 0,
		cmd_len,
		glyphcmds);

	free(glyphcmds);
}

static struct aui_geometry
aui_dri_get_text_geometry(struct primitive *p)
{
	struct aui_dri_xcb *dri_xcb = (struct aui_dri_xcb *)driver;
	struct aui_geometry geom = { 0 };
	struct text *t = (struct text *)p;
	size_t len = (t->data == NULL) ? 0 : strlen(t->data);

	for (int i = 0; i < len; i++) {
		uint32_t glyphid = (uint32_t)t->data[i];
		if (glyphid < 32 || glyphid > 126) {
			/* fprintf(stderr, "libaui: unknown geometry for glyph id: %d\n", glyphid); */
			continue;
		}

		struct glyph *g = &dri_xcb->glyphs[glyphid];
		geom.width += g->x_offset;
	}
	geom.height = dri_xcb->font_height;

	return geom;
}

/*
 * XCB section
 *
 * Function ends with xcb to prevent clashing with xcb
 * defined functions.
 */
static int
config_xcb(struct aui_dri_xcb *axcb)
{
	xcb_intern_atom_cookie_t cookies[2];

	axcb->conn = xcb_connect(NULL, NULL);

	if (xcb_connection_has_error(axcb->conn))
		return -1;

	axcb->screen = xcb_setup_roots_iterator(xcb_get_setup(axcb->conn)).data;

	if (axcb->screen == NULL)
		return -1;

	axcb->pollfd.fd = xcb_get_file_descriptor(axcb->conn);
	axcb->pollfd.events = POLLIN;

	/*
	 * Prevents auto delete
	 */
	cookies[0] = xcb_intern_atom(axcb->conn, 1, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
	axcb->wm_protocols = xcb_intern_atom_reply(axcb->conn, cookies[0], NULL);

	cookies[1] = xcb_intern_atom(axcb->conn, 1, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
	axcb->wm_delete_window = xcb_intern_atom_reply(axcb->conn, cookies[1], NULL);

	/*
	 * XCB Render
	 */
	xcb_render_query_pict_formats_cookie_t rcookie = xcb_render_query_pict_formats(axcb->conn);
	xcb_render_query_pict_formats_reply_t *rreply;
	xcb_render_pictscreen_iterator_t rpiter;

	rreply = xcb_render_query_pict_formats_reply(axcb->conn, rcookie, NULL);
	rpiter = xcb_render_query_pict_formats_screens_iterator(rreply);

	/*
	 * This is `NORMAL` format
	 */
	while (rpiter.rem != 0) {
		xcb_render_pictdepth_iterator_t rditer;
		rditer = xcb_render_pictscreen_depths_iterator(rpiter.data);

		while (rditer.rem != 0) {
			xcb_render_pictvisual_iterator_t rviter;
			rviter = xcb_render_pictdepth_visuals_iterator(rditer.data);

			while (rviter.rem != 0) {
				if (rviter.data->visual == axcb->screen->root_visual)
					axcb->fmt_normal = rviter.data->format;
				xcb_render_pictvisual_next(&rviter);
			}
			xcb_render_pictdepth_next(&rditer);
		}
		xcb_render_pictscreen_next(&rpiter);
	}

	/*
	 * Formats for `ARGB32` and `ALPHA8`
	 */
	xcb_render_pictforminfo_iterator_t rfiter;
	rfiter = xcb_render_query_pict_formats_formats_iterator(rreply);

	while (rfiter.rem != 0) {
		xcb_render_pictforminfo_t *info = rfiter.data;

		if (info->type == XCB_RENDER_PICT_TYPE_DIRECT &&
			info->depth == 32 &&
			info->direct.red_shift == 16 &&
			info->direct.red_mask == 0xff &&
			info->direct.green_shift == 8 &&
			info->direct.green_mask == 0xff &&
			info->direct.blue_shift == 0 &&
			info->direct.blue_mask == 0xff &&
			info->direct.alpha_shift == 24 &&
			info->direct.alpha_mask == 0xff)
			axcb->fmt_argb32 = rfiter.data->id;

		if (info->type == XCB_RENDER_PICT_TYPE_DIRECT &&
			info->depth == 8 &&
			info->direct.red_shift == 0 &&
			info->direct.red_mask == 0x00 &&
			info->direct.green_shift == 0 &&
			info->direct.green_mask == 0x00 &&
			info->direct.blue_shift == 0 &&
			info->direct.blue_mask == 0x00 &&
			info->direct.alpha_shift == 0 &&
			info->direct.alpha_mask == 0xff)
			axcb->fmt_alpha8 = rfiter.data->id;
		xcb_render_pictforminfo_next(&rfiter);
	}

	free(rreply);

	return 0;
}

/*
 * Sets pixmap. This is used when the window is being resized to fit with
 * the new dimensions
 */
static void
set_pixmap_xcb(struct aui_window_xcb *aw_xcb)
{
	struct aui_dri_xcb *dri_xcb = (struct aui_dri_xcb *)driver;
	struct aui_window *aw = (struct aui_window *)aw_xcb;
	struct aui_widget *ww = (struct aui_widget *)aw;

	xcb_render_free_picture(dri_xcb->conn, aw_xcb->dst_pict);
	xcb_render_free_picture(dri_xcb->conn, aw_xcb->src_pict);
	xcb_free_pixmap(dri_xcb->conn, aw_xcb->pixmap);

	aw_xcb->pixmap = xcb_generate_id(dri_xcb->conn);
	xcb_create_pixmap(dri_xcb->conn,
					  32,
					  aw_xcb->pixmap,
					  aw_xcb->xw,
					  ww->geom.width,
					  ww->geom.height);

	aw_xcb->src_pict = xcb_generate_id(dri_xcb->conn);
	xcb_render_create_picture(dri_xcb->conn,
							  aw_xcb->src_pict,
							  aw_xcb->pixmap,
							  dri_xcb->fmt_argb32,
							  0,
							  NULL);

	aw_xcb->dst_pict = xcb_generate_id(dri_xcb->conn);
	xcb_render_create_picture(dri_xcb->conn,
							  aw_xcb->dst_pict,
							  aw_xcb->xw,
							  dri_xcb->fmt_normal,
							  0,
							  NULL);
}

static struct aui_window_xcb*
find_window_xcb(xcb_window_t xw)
{
	struct aui_window_xcb *aw_xcb;
	struct aui_window *aw;

	LIST_FOREACH(aw, &wlist, entries) {
		aw_xcb = (struct aui_window_xcb *)aw;
		if (aw_xcb->xw == xw)
			return aw_xcb;
	}

	return NULL;
}

static void
set_name_xcb(struct aui_window_xcb *aw_xcb, const char *title)
{
	if (title == NULL) return;

	struct aui_dri_xcb *dri_xcb = (struct aui_dri_xcb *)driver;
	xcb_change_property(dri_xcb->conn,
						 XCB_PROP_MODE_REPLACE,
						 aw_xcb->xw,
						 XCB_ATOM_WM_NAME,
						 XCB_ATOM_STRING,
						 8,
						 strlen(title),
						 title
	);
}
