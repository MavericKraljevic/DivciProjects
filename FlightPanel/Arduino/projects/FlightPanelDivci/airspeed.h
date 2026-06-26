#pragma once
#include <lvgl.h>
#include <math.h>

#define DISPLAY_TITLE  "km/h"

#define LABEL_FRAC     3
#define LABEL_OFFSET_Y (240 - 480 / LABEL_FRAC)

#define NEEDLE_TIP_FRAC 10

// Speed range and zone limits (km/h)
#define IAS_MAX   300
#define IAS_VS    65
#define IAS_VFE   115
#define IAS_VNO   200
#define IAS_VNE   260

// Scale angles — degrees CW from 3 o'clock (90=bottom, 180=left, 270=top, 360=right, 450=bottom again…)
#define IAS_DEG_START   90   // angle where 0 km/h sits
#define IAS_DEG_SPIRAL  270  // angle where the ring starts winding inward
#define IAS_DEG_END     570  // angle where IAS_MAX sits

// Derived — don't edit
#define IAS_ROTATION     IAS_DEG_START
#define IAS_ARC_RANGE    (IAS_DEG_END - IAS_DEG_START)
#define IAS_SPIRAL_START ((float)(IAS_DEG_SPIRAL - IAS_DEG_START) * IAS_MAX / (IAS_DEG_END - IAS_DEG_START))

// Spiral geometry
#define GAUGE_R  240     // display radius in px — band outer edge sits at the physical edge
#define R_OUTER  (GAUGE_R - 3.0f)  // band center; ±3px half-width puts outer edge at display edge
#define R_INNER  120.0f  // inner ring radius at IAS_MAX (px)

static lv_obj_t *ias_meter;
static lv_obj_t *ias_canvas = NULL;
static float     ias_val = 0.0f;

static void ias_draw_needle(lv_draw_ctx_t *draw_ctx,
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

static inline float ias_r(float v) {
    if (v <= IAS_SPIRAL_START) return R_OUTER;
    float t = (v - IAS_SPIRAL_START) / (float)(IAS_MAX - IAS_SPIRAL_START);
    return R_OUTER + (R_INNER - R_OUTER) * t;
}

static inline float ias_a(float v) {
    return IAS_ROTATION + (v / (float)IAS_MAX) * IAS_ARC_RANGE;
}

static lv_color_t zone_color(int v) {
    if (v < IAS_VS)  return lv_color_hex(0x444444);
    if (v < IAS_VNO) return lv_color_hex(0x00BB00);
    if (v < IAS_VNE) return lv_color_hex(0xFFAA00);
    return lv_color_hex(0xFF2200);
}

// Draws the static scale (spiral + ticks) once into the canvas at startup.
// lv_canvas_draw_line takes an array of points, so we pass 2-element arrays.
static void draw_scale_to_canvas() {
    lv_draw_line_dsc_t ld;
    lv_draw_line_dsc_init(&ld);
    ld.opa = LV_OPA_COVER;

    // Colored spiral band
    ld.width       = 6;
    ld.round_start = 1;
    ld.round_end   = 1;
    for (int v = 0; v < IAS_MAX; v++) {
        ld.color = zone_color(v);
        float r1 = ias_r(v),     a1 = ias_a(v)     * (float)M_PI / 180.0f;
        float r2 = ias_r(v + 1), a2 = ias_a(v + 1) * (float)M_PI / 180.0f;
        lv_point_t pts[2] = {
            { (lv_coord_t)(240 + r1 * cosf(a1)), (lv_coord_t)(240 + r1 * sinf(a1)) },
            { (lv_coord_t)(240 + r2 * cosf(a2)), (lv_coord_t)(240 + r2 * sinf(a2)) }
        };
        lv_canvas_draw_line(ias_canvas, pts, 2, &ld);
    }

    // White inner stripe from VS to VFE
    ld.color = lv_color_white();
    ld.width = 4;
    for (int v = IAS_VS; v < IAS_VFE; v++) {
        float r1 = ias_r(v)     - 14.0f, a1 = ias_a(v)     * (float)M_PI / 180.0f;
        float r2 = ias_r(v + 1) - 14.0f, a2 = ias_a(v + 1) * (float)M_PI / 180.0f;
        lv_point_t pts[2] = {
            { (lv_coord_t)(240 + r1 * cosf(a1)), (lv_coord_t)(240 + r1 * sinf(a1)) },
            { (lv_coord_t)(240 + r2 * cosf(a2)), (lv_coord_t)(240 + r2 * sinf(a2)) }
        };
        lv_canvas_draw_line(ias_canvas, pts, 2, &ld);
    }

    // Tick marks
    ld.round_start = 0;
    ld.round_end   = 0;
    for (int v = 0; v <= IAS_MAX; v += 10) {
        bool  major    = (v % 50 == 0);
        float r        = ias_r(v);
        float a_rad    = ias_a(v) * (float)M_PI / 180.0f;
        float ca       = cosf(a_rad), sa = sinf(a_rad);
        float tick_len = major ? 22.0f : 11.0f;
        ld.color = lv_color_white();
        ld.width = major ? 3 : 2;
        lv_point_t pts[2] = {
            { (lv_coord_t)(240 + r             * ca), (lv_coord_t)(240 + r             * sa) },
            { (lv_coord_t)(240 + (r - tick_len)* ca), (lv_coord_t)(240 + (r - tick_len)* sa) }
        };
        lv_canvas_draw_line(ias_canvas, pts, 2, &ld);
    }
}

// Called every frame — only draws the needle on top of the static canvas.
static void ias_needle_cb(lv_event_t *e) {
    lv_draw_ctx_t *draw_ctx = lv_event_get_draw_ctx(e);
    const lv_coord_t cx = 240, cy = 240;

    float a = ias_a(ias_val);
    ias_draw_needle(draw_ctx, cx, cy, a, 210, 210/NEEDLE_TIP_FRAC, 30, 6, lv_color_white());

    lv_draw_rect_dsc_t cap;
    lv_draw_rect_dsc_init(&cap);
    cap.bg_color = lv_color_hex(0x333333);
    cap.bg_opa   = LV_OPA_COVER;
    cap.radius   = LV_RADIUS_CIRCLE;
    lv_area_t ca2 = { cx - 11, cy - 11, cx + 11, cy + 11 };
    lv_draw_rect(draw_ctx, &cap, &ca2);
}

void create_ias() {
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);

    ias_meter = lv_meter_create(lv_scr_act());
    lv_obj_set_size(ias_meter, 480, 480);
    lv_obj_align(ias_meter, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(ias_meter, lv_color_black(), 0);
    lv_obj_set_style_border_width(ias_meter, 0, 0);
    lv_obj_set_style_pad_all(ias_meter, 0, 0);

    // Canvas for the static scale — drawn once at startup, blitted each frame (fast).
    // 480×480 RGB565 = 460 800 bytes; must live in PSRAM.
    static lv_color_t *cbuf = NULL;
    bool first = (cbuf == NULL);
    if (first) cbuf = (lv_color_t *)heap_caps_malloc(LV_CANVAS_BUF_SIZE_TRUE_COLOR(480, 480), MALLOC_CAP_SPIRAM);
    ias_canvas = lv_canvas_create(ias_meter);
    lv_canvas_set_buffer(ias_canvas, cbuf, 480, 480, LV_IMG_CF_TRUE_COLOR);
    lv_obj_align(ias_canvas, LV_ALIGN_CENTER, 0, 0);
    if (first) {
        lv_canvas_fill_bg(ias_canvas, lv_color_black(), LV_OPA_COVER);
        draw_scale_to_canvas();
    }

    // Needle callback fires after all children (canvas + labels), so needle is on top.
    lv_obj_add_event_cb(ias_meter, ias_needle_cb, LV_EVENT_DRAW_POST_END, NULL);

    // Labels — created after canvas so they render on top of it.
    int label_vals[] = {0, 50, 100, 150, 200, 250, 300};
    char buf[8];
    for (int i = 0; i < 7; i++) {
        int v = label_vals[i];
        if (v > IAS_MAX) break;
        float r_label = ias_r(v) - 38.0f;
        float a_rad   = ias_a(v) * (float)M_PI / 180.0f;
        lv_obj_t *d = lv_label_create(ias_meter);
        snprintf(buf, sizeof(buf), "%d", v);
        lv_label_set_text(d, buf);
        lv_obj_set_style_text_color(d, lv_color_white(), 0);
        lv_obj_set_style_text_font(d, &lv_font_montserrat_32, 0);
        lv_obj_align(d, LV_ALIGN_CENTER,
                     (lv_coord_t)(r_label * cosf(a_rad)),
                     (lv_coord_t)(r_label * sinf(a_rad)));
    }

    lv_obj_t *title = lv_label_create(ias_meter);
    lv_label_set_text(title, DISPLAY_TITLE);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_36, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -LABEL_OFFSET_Y);

}

void update_ias(float val) {
    ias_val = val;
    lv_obj_invalidate(ias_meter);
}

