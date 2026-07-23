/*
 * Copyright (c) 2026 Onana Onana Xavier Manuel <xavier@onana.net>
 * 
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED AS IS AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <aui.h>

#define	DGT_MAX		9

struct equation {
	struct aui_label_config config;
	struct aui_label *label;
	char *display;
	int queue[2];
	int qindex;
	struct binfo *op;
};

struct acalc {
	struct equation equation;
	struct aui_window *aw;
};

enum button_class {
	CLASS_DGT,
	CLASS_DIV,
	CLASS_ADD,
	CLASS_SUB,
	CLASS_MUL,
	CLASS_EQU,
	CLASS_CLR,
};

struct binfo {
	enum button_class class;
	struct aui_button_config config;
	struct aui_gridpar par;
};

static struct acalc app;

static const struct aui_color theme[] = {
	{ 255, 255, 255, 255 },
};

static struct binfo binfo[] = {
	{ CLASS_CLR, { .text = "C", .foreground = theme[0] }, { .row = 0, .columnspan = 3 } },
	{ CLASS_DIV, { .text = "/", .foreground = theme[0] }, { .row = 0, .column = 3 } },

	{ CLASS_DGT, { .text = "7", .foreground = theme[0] }, { .row = 1, .column = 0 } },
	{ CLASS_DGT, { .text = "8", .foreground = theme[0] }, { .row = 1, .column = 1 } },
	{ CLASS_DGT, { .text = "9", .foreground = theme[0] }, { .row = 1, .column = 2 } },
	{ CLASS_MUL, { .text = "*", .foreground = theme[0] }, { .row = 1, .column = 3 } },

	{ CLASS_DGT, { .text = "4", .foreground = theme[0] }, { .row = 2, .column = 0 } },
	{ CLASS_DGT, { .text = "5", .foreground = theme[0] }, { .row = 2, .column = 1 } },
	{ CLASS_DGT, { .text = "6", .foreground = theme[0] }, { .row = 2, .column = 2 } },
	{ CLASS_SUB, { .text = "-", .foreground = theme[0] }, { .row = 2, .column = 3 } },

	{ CLASS_DGT, { .text = "1", .foreground = theme[0] }, { .row = 3, .column = 0 } },
	{ CLASS_DGT, { .text = "2", .foreground = theme[0] }, { .row = 3, .column = 1 } },
	{ CLASS_DGT, { .text = "3", .foreground = theme[0] }, { .row = 3, .column = 2 } },
	{ CLASS_ADD, { .text = "+", .foreground = theme[0] }, { .row = 3, .column = 3 } },

	{ CLASS_DGT, { .text = "0", .foreground = theme[0] }, { .row = 4, .columnspan = 3 } },
	{ CLASS_EQU, { .text = "=", .foreground = theme[0] }, { .row = 4, .column = 3 } },
};

void
display_init(struct aui_frame *frame)
{
	app.equation.display = malloc(DGT_MAX + 1);
	memset(app.equation.display, 0, DGT_MAX);

	app.equation.config = (struct aui_label_config) {
		.text = "What's up?",
		.foreground = { 0, 0, 0, 255 },
	};

	app.equation.label = aui_label_new(AUI_WIDGET(frame), &app.equation.config);
	aui_pack(AUI_WIDGET(app.equation.label), NULL);
	app.equation.op = NULL;
}

void
display_refresh()
{
	app.equation.config.text = app.equation.display;
	aui_label_config(AUI_WIDGET(app.equation.label), &app.equation.config);
}

void
display_empty()
{
	memset(app.equation.display, 0, DGT_MAX);
}

void
display_warn(const char *msg)
{
	display_empty();
	strncpy(app.equation.display, msg, strlen(msg));
	display_refresh();
	display_empty();
}

void
display_result(int res)
{
	display_empty();
	snprintf(app.equation.display, DGT_MAX, "%d", res);
	display_refresh();
	display_empty();
}

void
equation_add()
{
	const char *errstr;
	long long res;

	if (strlen(app.equation.display) == 0)
		return;

	res = strtonum(app.equation.display, 0, INT_MAX, &errstr);
	if (errstr != NULL) {
		fprintf(stderr, "acalc: invalid equation number added!\n");
		exit(1);
	}

	app.equation.queue[app.equation.qindex++] = (int)res;
}

void
equation_solve()
{
	long long res;

	if (app.equation.qindex < 2 || app.equation.op == NULL)
		return;

	switch (app.equation.op->class) {
	case CLASS_ADD:
		res = app.equation.queue[0] + app.equation.queue[1];
		if (res > 999999999 || res < 0) {
			display_warn("TOO BIG!");
			goto clear;
		}
		display_result(res);
		break;
	case CLASS_SUB:
		res = app.equation.queue[0] - app.equation.queue[1];
		if (res < 0 || res > 999999999) {
			display_warn("TOO SMALL!");
			goto clear;
		}
		display_result(res);
		break;
	case CLASS_MUL:
		res = app.equation.queue[0] * app.equation.queue[1];
		if (res > 999999999 || res < 0) {
			display_warn("TOO BIG!");
			goto clear;
		}
		display_result(res);
		break;
	case CLASS_DIV:
		if (app.equation.queue[1] == 0) {
			display_warn("NaN!");
			goto clear;
		}
		res = app.equation.queue[0] / app.equation.queue[1];
		if (res < 0 || res > 999999999) {
			display_warn("TOO SMALL!");
			goto clear;
		}
		display_result(res);
		break;
	default:
		break;
	}
	app.equation.queue[0] = res;
	app.equation.qindex = 1;
	return;

clear:
	app.equation.qindex = 0;
	display_empty();
}

void
write_dgt(const char *dgt)
{
	const char *errstr;
	long long num;
	int index;

	num = strtonum(dgt, 0, 9, &errstr);
	if (errstr != NULL) {
		fprintf(stderr, "acalc: out of bounds\n");
		exit(1);
	}

	index = strlen(app.equation.display);
	if (index == DGT_MAX)
		return;

	app.equation.display[index++] = dgt[0];
	app.equation.display[index] = '\0';
	display_refresh();
}

static void
button_cmd(struct aui_button *b, void *args)
{
	struct binfo *info = (struct binfo *)args;

	switch (info->class) {
	case CLASS_DGT:
		write_dgt(info->config.text);
		break;
	case CLASS_ADD:
	case CLASS_SUB:
	case CLASS_MUL:
	case CLASS_DIV:
		equation_add();
		equation_solve();
		app.equation.op = info;
		display_empty();
		break;
	case CLASS_CLR:
		display_empty();
		display_refresh();
		break;
	case CLASS_EQU:
		if (app.equation.op) {
			equation_add();
			equation_solve();
			app.equation.qindex = 0;
		}
		break;
	}
}

int
main()
{
	struct aui_frame *display_frame;
	struct aui_frame *buttons_frame;
	size_t bcount = sizeof(binfo) / sizeof(*binfo);

	{
		struct aui_window_config config = { .title = "acalc", .width = 120, .height = 188 };
		app.aw = aui_window_new(&config);
	}

	{
		struct aui_frame_config config = { .background = { 255, 255, 255, 255 } };
		display_frame = aui_frame_new(AUI_WIDGET(app.aw), &config);
	}

	{
		struct aui_frame_config config = { .background = { 0, 0, 0, 255 } };
		buttons_frame = aui_frame_new(AUI_WIDGET(app.aw), &config);
	}

	struct aui_placepar place[] = {
		{ .relwidth = 1.0, .relheight = 0.2 },
		{ .rely = 0.2, .relwidth = 1.0, .relheight = 0.8 }
	};

	aui_place(AUI_WIDGET(display_frame), &place[0]);
	aui_place(AUI_WIDGET(buttons_frame), &place[1]);
	display_init(display_frame);

	for (int i = 0; i < bcount; i++) {
		struct binfo info = binfo[i];

		info.config.command = &button_cmd;
		info.config.args = &binfo[i];

		struct aui_button *btn = aui_button_new(AUI_WIDGET(buttons_frame), &info.config);
		aui_grid(AUI_WIDGET(btn), &binfo[i].par);
	}

	aui_run();
	return 0;
}
