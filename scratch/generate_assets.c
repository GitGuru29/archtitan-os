#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cairo.h>

#define ASSET_DIR "airootfs/usr/share/plymouth/themes/archtitan-rise/assets"

static void ensure_dir(const char *path) {
    mkdir("airootfs/usr/share/plymouth/themes/archtitan-rise", 0755);
    mkdir(path, 0755);
}

/* Render glowing dot sprite (128x128) */
static void generate_dot_png() {
    int w = 128, h = 128;
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    cairo_t *cr = cairo_create(surface);

    cairo_pattern_t *pat = cairo_pattern_create_radial(64.0, 64.0, 0.0, 64.0, 64.0, 60.0);
    cairo_pattern_add_color_stop_rgba(pat, 0.0,  1.0, 1.0, 1.0, 1.0);     /* White core */
    cairo_pattern_add_color_stop_rgba(pat, 0.4,  0.72, 0.80, 1.0, 0.9);   /* Soft blue */
    cairo_pattern_add_color_stop_rgba(pat, 0.8,  0.48, 0.63, 0.97, 0.4);   /* Violet halo */
    cairo_pattern_add_color_stop_rgba(pat, 1.0,  0.0, 0.0, 0.0, 0.0);     /* Transparent edge */

    cairo_set_source(cr, pat);
    cairo_arc(cr, 64.0, 64.0, 60.0, 0, 2 * M_PI);
    cairo_fill(cr);

    cairo_pattern_destroy(pat);
    cairo_destroy(cr);

    char outpath[512];
    snprintf(outpath, sizeof(outpath), "%s/dot.png", ASSET_DIR);
    cairo_surface_write_to_png(surface, outpath);
    cairo_surface_destroy(surface);
}

/* Render vertically stretched streak sprite (128x320) */
static void generate_dot_stretched_png() {
    int w = 128, h = 320;
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    cairo_t *cr = cairo_create(surface);

    cairo_pattern_t *pat = cairo_pattern_create_linear(64.0, 0.0, 64.0, 320.0);
    cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.0, 0.0, 0.0, 0.0);
    cairo_pattern_add_color_stop_rgba(pat, 0.5, 0.85, 0.92, 1.0, 0.95);
    cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.0, 0.0, 0.0, 0.0);

    cairo_set_source(cr, pat);
    cairo_save(cr);
    cairo_translate(cr, 64.0, 160.0);
    cairo_scale(cr, 30.0, 150.0);
    cairo_arc(cr, 0.0, 0.0, 1.0, 0, 2 * M_PI);
    cairo_restore(cr);
    cairo_fill(cr);

    cairo_pattern_destroy(pat);
    cairo_destroy(cr);

    char outpath[512];
    snprintf(outpath, sizeof(outpath), "%s/dot_stretched.png", ASSET_DIR);
    cairo_surface_write_to_png(surface, outpath);
    cairo_surface_destroy(surface);
}

/* Render individual letter glyph PNG (512px height cap) */
static void generate_glyph_png(const char *name, const char *text) {
    int w = 400, h = 512;
    if (text[0] == ',' || text[0] == '.') {
        w = 180;
    }
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    cairo_t *cr = cairo_create(surface);

    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 360.0);

    cairo_text_extents_t extents;
    cairo_text_extents(cr, text, &extents);

    double x = (w - extents.width) / 2.0 - extents.x_bearing;
    double y = (h - extents.height) / 2.0 - extents.y_bearing;

    cairo_set_source_rgba(cr, 0.95, 0.97, 1.0, 1.0); /* Crisp off-white */
    cairo_move_to(cr, x, y);
    cairo_show_text(cr, text);

    cairo_destroy(cr);

    char outpath[512];
    snprintf(outpath, sizeof(outpath), "%s/glyph_%s.png", ASSET_DIR, name);
    cairo_surface_write_to_png(surface, outpath);
    cairo_surface_destroy(surface);
}

/* Render EXPLORE NOW interactive button (360x72 px) */
static void generate_button_png() {
    int w = 360, h = 72;
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    cairo_t *cr = cairo_create(surface);

    /* Rounded pill background */
    double aspect = 1.0, x = 4.0, y = 4.0, width = w - 8.0, height = h - 8.0, corner_radius = 32.0;
    double radius = corner_radius / aspect;
    double degrees = M_PI / 180.0;

    cairo_new_sub_path(cr);
    cairo_arc(cr, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);
    cairo_arc(cr, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
    cairo_arc(cr, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);
    cairo_arc(cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
    cairo_close_path(cr);

    /* Semi-transparent dark background */
    cairo_set_source_rgba(cr, 0.10, 0.12, 0.18, 0.85);
    cairo_fill_preserve(cr);

    /* Glowing cyan border */
    cairo_set_source_rgba(cr, 0.48, 0.63, 0.97, 0.80);
    cairo_set_line_width(cr, 2.0);
    cairo_stroke(cr);

    /* Text: EXPLORE NOW -> */
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 22.0);

    cairo_text_extents_t extents;
    const char *label = "EXPLORE NOW  \xe2\x86\x92";
    cairo_text_extents(cr, label, &extents);

    double tx = (w - extents.width) / 2.0 - extents.x_bearing;
    double ty = (h - extents.height) / 2.0 - extents.y_bearing;

    cairo_set_source_rgba(cr, 0.95, 0.97, 1.0, 1.0);
    cairo_move_to(cr, tx, ty);
    cairo_show_text(cr, label);

    cairo_destroy(cr);

    char outpath[512];
    snprintf(outpath, sizeof(outpath), "%s/btn_explore.png", ASSET_DIR);
    cairo_surface_write_to_png(surface, outpath);
    cairo_surface_destroy(surface);
}

int main(void) {
    printf("[ArchTitan C Asset Generator] Building Rise, Titan. Plymouth assets...\n");
    ensure_dir(ASSET_DIR);

    generate_dot_png();
    generate_dot_stretched_png();
    generate_button_png();

    struct { const char *name; const char *text; } glyphs[] = {
        {"R", "R"}, {"i", "i"}, {"s", "s"}, {"e", "e"},
        {"comma", ","}, {"T", "T"}, {"t", "t"}, {"a", "a"},
        {"n", "n"}, {"period", "."}
    };

    for (size_t i = 0; i < sizeof(glyphs) / sizeof(glyphs[0]); i++) {
        generate_glyph_png(glyphs[i].name, glyphs[i].text);
    }

    printf("[ArchTitan C Asset Generator] Successfully generated all PNG assets in %s\n", ASSET_DIR);
    return 0;
}
