#include <fontconfig/fontconfig.h>
#include <ft2build.h>
#include FT_FREETYPE_H

struct _font {
    FcConfig   *fc_config;
    FT_Library  ft_library;
};

static struct _font font;

static char *find_font_fc(const char *);

int
font_init()
{
    char *path;

    font.fc_config = FcInitLoadConfigAndFonts();
    if (font.fc_config == NULL) {
        fprintf(stderr, "libaui: failed to load fontconfig\n");
        return -1;
    }

    path = find_font_fc("sans-serif");

    if (FT_Init_FreeType(&font.ft_library) != 0) {
        fprintf(stderr, "libaui: FT_Init_FreeType failed!\n");
        return -1;
    }

    free(path);
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
