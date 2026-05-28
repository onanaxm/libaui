#ifndef __AUI_EVENT_H__
#define __AUI_EVENT_H__

#include "openbsd-queue.h"

/*
 * These aren't accessible in aui.h because there is no
 * need for events. While using the library, you should
 * handle interactions through callback functions
 *
 * These are used internaly.
 */

enum aui_event_type {
    AUI_EVENT_QUIT,
    AUI_EVENT_RESIZE,
    AUI_EVENT_KEY_PRESS,
    AUI_EVENT_MOUSE_PRESS,
    AUI_EVENT_MOUSE_RELEASE,
    AUI_EVENT_MOUSE_MOTION,
};

TAILQ_HEAD(aui_event_queue, aui_event);
struct aui_event {
    int type;
    struct aui_window *aw;
    TAILQ_ENTRY(aui_event) entries;
};

struct aui_event_quit {
    struct aui_event event;
};

struct aui_event_key_press {
    struct aui_event event;
    unsigned int key;
};

struct aui_event_resize {
    struct aui_event event;
    uint16_t old_w, old_h;
    uint16_t new_w, new_h;
};

struct aui_event_mouse_motion {
    struct aui_event event;
    uint16_t x, y;
};

struct aui_event_mouse_press {
    struct aui_event event;
    uint16_t x, y;
    uint8_t button;
};

struct aui_event_mouse_release {
    struct aui_event event;
    uint16_t x, y;
    uint8_t button;
};

extern struct aui_event_queue ev_queue;

#endif
