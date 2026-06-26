#pragma once
#include <lvgl.h>
#include <math.h>

#define DISPLAY_TITLE  "ALT"
#define DISPLAY_UNIT   "m"

#define LABEL_FRAC     3
#define LABEL_OFFSET_Y (240 - 480 / LABEL_FRAC)

#define NEEDLE_TIP_FRAC 10

#define GAUGE_R 240  // display radius in px — ticks start at the physical edge

static lv_obj_t *alt_meter;
static lv_obj_t *alt_canvas = NULL;
static float alt_val_m  = 0.0f;
static float alt_val_km = 0.0f;

static void alt_draw_needle(lv_draw_ctx_t *draw_ctx,
                         lv_coord_t cx, lv_coord_t cy, float angle_deg,
                         lv_coord_t length, lv_coord_t tip_len, lv_coord_t tail, lv_coord_t hw,
                         lv_color_t color) {
    float a  = angle_deg * (float)M_PI / 180.0f;
    float ca = cosf(a), sa = sinf(a);
    lv_coord_t shoulder = length - tip_len;
    lv_point_t pts[5] = {
        { (lv_coord_t)(cx + length   * ca),               (lv_coord_t)(cy + length   * sa) },
        { (lv_coord_t)(cx + shoulder * ca - hw * sa),     (lv_coord_t)(cy + shoulder * sa + hw * ca) },
        { (lv_coord_t)(cx - tail     * ca - hw * sa),     (lv_coord_t)(cy - tail     * sa + hw * ca) },
        { (lv_coord_t)(cx - tail     * ca + hw * sa),     (lv_coord_t)(cy - tail     * sa - hw * ca) },
        { (lv_coord_t)(cx + shoulder * ca + hw * sa),     (lv_coord_t)(cy + shoulder * sa - hw * ca) },
    };
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = color;
    dsc.bg_opa   = LV_OPA_COVER;
    lv_draw_polygon(draw_ctx, &dsc, pts, 5);
}

static void draw_alt_scale_to_canvas() {
    lv_draw_line_dsc_t ld;
    lv_draw_line_dsc_init(&ld);
    ld.opa       = LV_OPA_COVER;
    ld.round_start = 0;
    ld.round_end   = 0;

    // Outer scale: 0–1000 m, 101 ticks every 10 m, major every 100 m.
    // 90° = bottom (0 m), going CW.
    for (int i = 0; i <= 100; i++) {
        bool  major   = (i % 10 == 0);
        float a_deg   = 90.0f + (i / 100.0f) * 360.0f;
        float a_rad   = a_deg * (float)M_PI / 180.0f;
        float ca = cosf(a_rad), sa = sinf(a_rad);
        float r_out   = (float)GAUGE_R;
        float tick_len = major ? 42.0f : 22.0f;
        ld.color = major ? lv_color_white() : lv_color_hex(0x666666);
        ld.width = major ? 9 : 4;
        lv_point_t pts[2] = {
            { (lv_coord_t)(240 + r_out             * ca), (lv_coord_t)(240 + r_out             * sa) },
            { (lv_coord_t)(240 + (r_out - tick_len) * ca), (lv_coord_t)(240 + (r_out - tick_len) * sa) }
        };
        lv_canvas_draw_line(alt_canvas, pts, 2, &ld);
    }

    // Inner scale: 0–10 km, 11 ticks every 1 km.
    for (int i = 0; i <= 10; i++) {
        float a_deg   = 90.0f + (i / 10.0f) * 360.0f;
        float a_rad   = a_deg * (float)M_PI / 180.0f;
        float ca = cosf(a_rad), sa = sinf(a_rad);
        float r_out   = (float)GAUGE_R;
        ld.color = lv_color_hex(0x444444);
        ld.width = 5;
        lv_point_t pts[2] = {
            { (lv_coord_t)(240 + r_out        * ca), (lv_coord_t)(240 + r_out        * sa) },
            { (lv_coord_t)(240 + (r_out - 18) * ca), (lv_coord_t)(240 + (r_out - 18) * sa) }
        };
        lv_canvas_draw_line(alt_canvas, pts, 2, &ld);
    }

    // Digit labels 0–9 at radius 178 px, 0 at bottom going CW.
    static const char *digits[] = {"0","1","2","3","4","5","6","7","8","9"};
    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.opa   = LV_OPA_COVER;
    label_dsc.color = lv_color_white();
    label_dsc.font  = &lv_font_montserrat_48;

    for (int i = 0; i < 10; i++) {
        float rad = (180.0f + i * 36.0f) * (float)M_PI / 180.0f;
        lv_coord_t cx_l = (lv_coord_t)(240 + 178.0f * sinf(rad));
        lv_coord_t cy_l = (lv_coord_t)(240 - 178.0f * cosf(rad));
        lv_point_t sz;
        lv_txt_get_size(&sz, digits[i], label_dsc.font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        lv_canvas_draw_text(alt_canvas,
                            cx_l - sz.x / 2,
                            cy_l - sz.y / 2,
                            sz.x + 4, &label_dsc, digits[i]);
    }

    // Title and unit
    lv_draw_label_dsc_t title_dsc;
    lv_draw_label_dsc_init(&title_dsc);
    title_dsc.opa   = LV_OPA_COVER;
    title_dsc.color = lv_color_white();
    title_dsc.font  = &lv_font_montserrat_36;
    lv_point_t tsz;
    lv_txt_get_size(&tsz, DISPLAY_TITLE, title_dsc.font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    lv_canvas_draw_text(alt_canvas,
                        240 - tsz.x / 2,
                        240 - LABEL_OFFSET_Y - tsz.y / 2,
                        tsz.x + 4, &title_dsc, DISPLAY_TITLE);

    if (strlen(DISPLAY_UNIT) > 0) {
        lv_draw_label_dsc_t unit_dsc;
        lv_draw_label_dsc_init(&unit_dsc);
        unit_dsc.opa   = LV_OPA_COVER;
        unit_dsc.color = lv_color_hex(0xAAAAAA);
        unit_dsc.font  = &lv_font_montserrat_32;
        lv_point_t usz;
        lv_txt_get_size(&usz, DISPLAY_UNIT, unit_dsc.font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        lv_canvas_draw_text(alt_canvas,
                            240 - usz.x / 2,
                            240 + LABEL_OFFSET_Y - usz.y / 2,
                            usz.x + 4, &unit_dsc, DISPLAY_UNIT);
    }
}

static void alt_needle_cb(lv_event_t *e) {
    lv_draw_ctx_t *draw_ctx = lv_event_get_draw_ctx(e);
    const lv_coord_t cx = 240, cy = 240;

    float a_m  = 90.0f + (alt_val_m  / 1000.0f) * 360.0f;
    float a_km = 90.0f + (alt_val_km / 10.0f)   * 360.0f;

    alt_draw_needle(draw_ctx, cx, cy, a_m,  210, 210/NEEDLE_TIP_FRAC, 30, 6, lv_color_white());
    alt_draw_needle(draw_ctx, cx, cy, a_km, 130, 130/NEEDLE_TIP_FRAC, 22, 9, lv_color_white());

    lv_draw_rect_dsc_t cap;
    lv_draw_rect_dsc_init(&cap);
    cap.bg_color = lv_color_hex(0x333333);
    cap.bg_opa   = LV_OPA_COVER;
    cap.radius   = LV_RADIUS_CIRCLE;
    lv_area_t ca = { cx - 11, cy - 11, cx + 11, cy + 11 };
    lv_draw_rect(draw_ctx, &cap, &ca);
}

void create_alt() {
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);

    alt_meter = lv_meter_create(lv_scr_act());
    lv_obj_set_size(alt_meter, 480, 480);
    lv_obj_align(alt_meter, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(alt_meter, lv_color_black(), 0);
    lv_obj_set_style_border_width(alt_meter, 0, 0);
    lv_obj_set_style_pad_all(alt_meter, 0, 0);

    // Canvas for static scale — drawn once, blitted each frame.
    static lv_color_t *cbuf = NULL;
    bool first = (cbuf == NULL);
    if (first) cbuf = (lv_color_t *)heap_caps_malloc(LV_CANVAS_BUF_SIZE_TRUE_COLOR(480, 480), MALLOC_CAP_SPIRAM);
    alt_canvas = lv_canvas_create(alt_meter);
    lv_canvas_set_buffer(alt_canvas, cbuf, 480, 480, LV_IMG_CF_TRUE_COLOR);
    lv_obj_align(alt_canvas, LV_ALIGN_CENTER, 0, 0);
    if (first) {
        lv_canvas_fill_bg(alt_canvas, lv_color_black(), LV_OPA_COVER);
        draw_alt_scale_to_canvas();
    }

    lv_obj_add_event_cb(alt_meter, alt_needle_cb, LV_EVENT_DRAW_POST_END, NULL);
}

void update_alt(float val) {
    alt_val_m  = fmodf(val, 1000.0f);
    alt_val_km = val / 1000.0f;
    lv_obj_invalidate(alt_meter);
}

