#include <stdio.h>
#include <stdlib.h>

#include "driver.h"

int
driver_open(enum driver_type type)
{
	switch (type) {
	case DRIVER_TYPE_X11:
		xcb_driver_open();
		break;
	default:
		fprintf(stderr, "libaui: attempt to open unknown driver\n");
		return -1;
	}
	return 0;
}
