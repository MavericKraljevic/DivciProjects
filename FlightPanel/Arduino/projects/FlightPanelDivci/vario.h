#pragma once
#include <lvgl.h>
#include <math.h>

#define DISPLAY_TITLE  "V/S"
#define DISPLAY_UNIT   "m/s"

#define LABEL_FRAC     3
#define LABEL_OFFSET_Y (240 - 480 / LABEL_FRAC)

#define NEEDLE_TIP_FRAC 10

#define GAUGE_R 240  // display radius in px — ticks start at the physical edge

static lv_obj_t *var_meter;
static lv_obj_t *var_canvas = NULL;
static float var_val = 0.0f;

static void var_draw_needle(lv_draw_ctx_t *draw_ctx,
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

static void draw_vario_scale_to_canvas() {
    lv_draw_line_dsc_t ld;
    lv_draw_line_dsc_init(&ld);
    ld.opa       = LV_OPA_COVER;
    ld.round_start = 0;
    ld.round_end   = 0;

    // 41 ticks from -20 to +20 m/s
    for (int i = 0; i <= 40; i++) {
        float v       = -20.0f + i;
        bool  major   = (((int)fabsf(v)) % 5 == 0);
        float a_rad   = ((v + 20.0f) / 40.0f) * 2.0f * (float)M_PI;
        float ca = cosf(a_rad), sa = sinf(a_rad);
        float r_out   = (float)GAUGE_R;
        float tick_len = major ? 36.0f : 18.0f;
        ld.color = major ? lv_color_white() : lv_color_hex(0x666666);
        ld.width = major ? 8 : 3;
        lv_point_t pts[2] = {
            { (lv_coord_t)(240 + r_out            * ca), (lv_coord_t)(240 + r_out            * sa) },
            { (lv_coord_t)(240 + (r_out - tick_len) * ca), (lv_coord_t)(240 + (r_out - tick_len) * sa) }
        };
        lv_canvas_draw_line(var_canvas, pts, 2, &ld);
    }

    // Number labels
    const char *texts[] = { "20", "15", "10",  "5",  "0",  "5", "10", "15" };
    const int   vals[]  = {  -20,  -15,  -10,   -5,    0,    5,   10,   15  };
    const bool  big[]   = { true, false, true, false, true, false, true, false };

    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.opa = LV_OPA_COVER;

    for (int i = 0; i < 8; i++) {
        float a_rad = ((vals[i] + 20.0f) / 40.0f) * 2.0f * (float)M_PI;
        label_dsc.color = lv_color_white();
        label_dsc.font  = big[i] ? &lv_font_montserrat_48 : &lv_font_montserrat_32;
        lv_point_t sz;
        lv_txt_get_size(&sz, texts[i], label_dsc.font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        lv_coord_t tx = (lv_coord_t)(240 + 170.0f * cosf(a_rad)) - sz.x / 2;
        lv_coord_t ty = (lv_coord_t)(240 + 170.0f * sinf(a_rad)) - sz.y / 2;
        lv_canvas_draw_text(var_canvas, tx, ty, sz.x + 4, &label_dsc, texts[i]);
    }

    // Title and unit
    lv_draw_label_dsc_t title_dsc;
    lv_draw_label_dsc_init(&title_dsc);
    title_dsc.opa   = LV_OPA_COVER;
    title_dsc.color = lv_color_white();
    title_dsc.font  = &lv_font_montserrat_36;
    lv_point_t tsz;
    lv_txt_get_size(&tsz, DISPLAY_TITLE, title_dsc.font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    lv_canvas_draw_text(var_canvas,
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
        lv_canvas_draw_text(var_canvas,
                            240 - usz.x / 2,
                            240 + LABEL_OFFSET_Y - usz.y / 2,
                            usz.x + 4, &unit_dsc, DISPLAY_UNIT);
    }
}

static void var_needle_cb(lv_event_t *e) {
    lv_draw_ctx_t *draw_ctx = lv_event_get_draw_ctx(e);
    const lv_coord_t cx = 240, cy = 240;

    float v = fmaxf(-19.9f, fminf(19.9f, var_val));
    float a = ((v + 20) / 40.0f) * 360.0f;
    var_draw_needle(draw_ctx, cx, cy, a, 210, 210/NEEDLE_TIP_FRAC, 30, 6, lv_color_white());

    lv_draw_rect_dsc_t cap;
    lv_draw_rect_dsc_init(&cap);
    cap.bg_color = lv_color_hex(0x333333);
    cap.bg_opa   = LV_OPA_COVER;
    cap.radius   = LV_RADIUS_CIRCLE;
    lv_area_t ca2 = { cx - 11, cy - 11, cx + 11, cy + 11 };
    lv_draw_rect(draw_ctx, &cap, &ca2);
}

void create_vario() {
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);

    var_meter = lv_meter_create(lv_scr_act());
    lv_obj_set_size(var_meter, 480, 480);
    lv_obj_align(var_meter, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(var_meter, lv_color_black(), 0);
    lv_obj_set_style_border_width(var_meter, 0, 0);
    lv_obj_set_style_pad_all(var_meter, 0, 0);

    static lv_color_t *cbuf = NULL;
    bool first = (cbuf == NULL);
    if (first) cbuf = (lv_color_t *)heap_caps_malloc(LV_CANVAS_BUF_SIZE_TRUE_COLOR(480, 480), MALLOC_CAP_SPIRAM);
    var_canvas = lv_canvas_create(var_meter);
    lv_canvas_set_buffer(var_canvas, cbuf, 480, 480, LV_IMG_CF_TRUE_COLOR);
    lv_obj_align(var_canvas, LV_ALIGN_CENTER, 0, 0);
    if (first) {
        lv_canvas_fill_bg(var_canvas, lv_color_black(), LV_OPA_COVER);
        draw_vario_scale_to_canvas();
    }

    lv_obj_add_event_cb(var_meter, var_needle_cb, LV_EVENT_DRAW_POST_END, NULL);
}

void update_vario(float val) {
    var_val = val;
    lv_obj_invalidate(var_meter);
}

