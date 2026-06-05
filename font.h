#ifndef FONT_H

struct glyph {
    unsigned int charcode;
    int x, y;
    unsigned int width;
    unsigned int height;
    int x_offset;
    int y_offset;
    unsigned int stride;
    unsigned char *bitmap;
};

int font_init(void);
int font_load_glyphs(struct glyph *, unsigned int);

#endif
