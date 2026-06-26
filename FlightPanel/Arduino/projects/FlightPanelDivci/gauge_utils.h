#pragma once
#include <lvgl.h>
#include <math.h>

static inline void draw_needle(lv_draw_ctx_t *draw_ctx,
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
