#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "event.h"
#include "driver.h"
#include "widget.h"
#include "layout.h"

struct aui_event_queue ev_queue;

void
aui_add_event(struct aui_event *event)
{
	struct aui_event *toadd;

	switch (event->type) {
	case AUI_EVENT_QUIT: {
		struct aui_event_quit *qev = calloc(1, sizeof(struct aui_event_quit));
		memcpy((void *)qev, event, sizeof(struct aui_event_quit));

		toadd = &qev->event;
		break;
	}
	case AUI_EVENT_RESIZE: {
		struct aui_event_resize *erz = calloc(1, sizeof(struct aui_event_resize));
		memcpy((void *)erz, event, sizeof(struct aui_event_resize));

		/*
		printf("libaui: resize %dx%d => %dx%d\n", erz->old_w, erz->old_h, erz->new_w, erz->new_h);
		*/

		toadd = &erz->event;
		break;
	}
	case AUI_EVENT_MOUSE_MOTION: {
		struct aui_event_mouse_motion *mev = calloc(1, sizeof(struct aui_event_mouse_motion));
		memcpy((void *)mev, event, sizeof(struct aui_event_mouse_motion));

		toadd = &mev->event;
		break;
	}
	case AUI_EVENT_MOUSE_PRESS: {
		struct aui_event_mouse_press *mpe = calloc(1, sizeof(struct aui_event_mouse_press));
		memcpy((void *)mpe, event, sizeof(struct aui_event_mouse_press));

		toadd = &mpe->event;
		break;
	}
	case AUI_EVENT_MOUSE_RELEASE: {
		struct aui_event_mouse_release *mre = calloc(1, sizeof(struct aui_event_mouse_release));
		memcpy((void *)mre, event, sizeof(struct aui_event_mouse_release));

		toadd = &mre->event;
		break;
	}
	default:
		fprintf(stderr, "libaui: failed to add event\n");
		return;
	}

	TAILQ_INSERT_TAIL(&ev_queue, toadd, entries);
}

void
aui_run(void)
{
	while (!LIST_EMPTY(&wlist)) {
		driver->ops->handle_events();

		/*
		 * Event Management
		 */
		struct aui_event *ev;
		while (!TAILQ_EMPTY(&ev_queue)) {
			ev = TAILQ_FIRST(&ev_queue);
			TAILQ_REMOVE(&ev_queue, ev, entries);

			switch (ev->type) {
			case AUI_EVENT_QUIT: {
				struct aui_event_quit *qev = (struct aui_event_quit *)ev;
				struct aui_window *aw = ev->aw;
				struct aui_widget *ww = (struct aui_widget *)aw;

				ww->ops->free(ww);

				free((void *)qev);
				break;
			}
			case AUI_EVENT_RESIZE: {
				struct aui_event_resize *rze = (struct aui_event_resize *)ev;
				struct aui_window *aw = ev->aw;

				aw->config.width = rze->new_w;
				aw->config.height = rze->new_h;

				driver->ops->resize_window(aw);
				free((void *)rze);

				layout_organize((struct aui_widget *)aw);
				break;
			}
			case AUI_EVENT_MOUSE_MOTION: {
				struct aui_event_mouse_motion *mev = (struct aui_event_mouse_motion *)ev;
				struct aui_window *aw = ev->aw;
				struct aui_widget *ww = (struct aui_widget *)aw;
				struct widget_ops *ops = ww->ops;

				ops->mouse_hover(ww, mev->x, mev->y);
				free((void *)mev);
				break;
			}
			case AUI_EVENT_MOUSE_PRESS: {
				struct aui_event_mouse_press *mpe = (struct aui_event_mouse_press *)ev;
				struct aui_window *aw = ev->aw;
				struct aui_widget *ww = (struct aui_widget *)aw;
				struct widget_ops *ops = ww->ops;

				ops->mouse_press(ww, mpe->x, mpe->y, mpe->button);
				free((void *)mpe);
				break;
			}
			case AUI_EVENT_MOUSE_RELEASE: {
				struct aui_event_mouse_release *mre = (struct aui_event_mouse_release *)ev;
				struct aui_window *aw = ev->aw;
				struct aui_widget *ww = (struct aui_widget *)aw;
				struct widget_ops *ops = ww->ops;

				ops->mouse_release(ww, mre->x, mre->y, mre->button);
				free((void *)mre);
				break;
			}
			default:
				break;
			}
		}

		/*
		 * Draw Calls
		 */
		struct aui_window *aw;
		LIST_FOREACH(aw, &wlist, entries) {
			if (aw->draw_flag) {
				struct aui_widget *widget = (struct aui_widget *)aw;
				driver->ops->render_background(aw);
				widget_draw(widget);
				driver->ops->render_foreground(aw);
				aw->draw_flag = 0;
			}
		}
	}
}

void
aui_destroy(struct aui_widget *widget)
{
	widget->ops->free(widget);
}
