#ifndef PRIMITIVE_H
#define PRIMITIVE_H

enum primitive_type {
    PRIMITIVE_TYPE_RECTANGLE,
    PRIMITIVE_TYPE_TEXT,
};

struct primitive {
    unsigned int type;
};

#endif
