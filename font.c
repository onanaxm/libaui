#include <fontconfig/fontconfig.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "font.h"

struct _font {
	FcConfig	   *fc_config;
	FT_Library	  ft_library;
	char		   *base;
	unsigned int	size;
};

static struct _font font;

static char *find_font_fc(const char *);

int
font_init()
{
	font.fc_config = FcInitLoadConfigAndFonts();
	if (font.fc_config == NULL) {
		fprintf(stderr, "libaui: failed to load fontconfig\n");
		return -1;
	}

	font.size = 12;
	font.base = find_font_fc("sans-serif");

	if (FT_Init_FreeType(&font.ft_library) != 0) {
		fprintf(stderr, "libaui: FT_Init_FreeType failed!\n");
		return -1;
	}

	return 0;
}

int
font_load_glyphs(struct glyph *gptr, unsigned int dt)
{
	FT_Face face;

	if (FT_New_Face(font.ft_library, font.base, 0, &face) != 0 ) {
		fprintf(stderr, "libaui: failed to load new face\n");
		return -1;
	}

	if (FT_Select_Charmap(face, FT_ENCODING_UNICODE) != 0) {
		fprintf(stderr, "libaui: FT_SelectCharmap failed\n");
		return -1;
	}

	FT_Set_Pixel_Sizes(face, 0, font.size);

	for (int i = 0; i < dt; i++) {
		struct glyph *g = &gptr[i];
		if (FT_Load_Char(face, g->charcode, FT_LOAD_RENDER) != 0) {
			fprintf(stderr, "libaui: FT_Load_Char failed\n");
			return -1;
		}

		FT_Bitmap bmp = face->glyph->bitmap;

		g->x = -face->glyph->bitmap_left;
		g->y =  face->glyph->bitmap_top;
		g->height = bmp.rows;
		g->width = bmp.width;
		g->x_offset = face->glyph->advance.x >> 6; /* 26.6 format to pixels */
		g->y_offset = face->glyph->metrics.horiBearingY >> 6;
		g->stride = (g->width + 3) &~3;
		g->bitmap = calloc(g->stride * g->height, sizeof(*g->bitmap));

		for (int y = 0; y < g->height; y++)
			memcpy(g->bitmap + y * g->stride, bmp.buffer + y * g->width, g->width);
	}

	FT_Done_Face(face);
	return 0;
}

static char*
find_font_fc(const char *name)
{
	FcPattern *pattern;
	FcPattern *match;
	FcResult result;
	FcChar8 *file;
	char *path;

	pattern  = FcNameParse((FcChar8 *)name);
	if (pattern == NULL)
		return NULL;

	if (FcConfigSubstitute(font.fc_config, pattern, FcMatchPattern) == FcFalse) {
		fprintf(stderr, "libaui: FcConfigSubstitute failed!\n");
		return NULL;
	}

	FcConfigSetDefaultSubstitute(font.fc_config, pattern);
	match = FcFontMatch(font.fc_config, pattern, &result);

	if (FcPatternGetString(match, FC_FILE, 0, &file) == FcResultTypeMismatch) {
		fprintf(stderr, "libaui: FcPatternGetString\n");
		return NULL;
	}

	path = strdup((char *)file);
	FcPatternDestroy(match);
	FcPatternDestroy(pattern);
	return path;
}
