#pragma once
#include <cstdarg>
#include <cstddef>
#include <cstdint>
struct lv_obj_t {};
struct lv_event_t {};
struct lv_indev_t {};
struct lv_font_t {};
struct lv_style_t {};
struct lv_color_t { std::uint16_t full; };
using lv_coord_t = std::int16_t;
using lv_event_code_t = int;
struct lv_point_t { lv_coord_t x; lv_coord_t y; };
inline lv_font_t lv_font_montserrat_14;
constexpr int LV_OPA_COVER=255, LV_OPA_TRANSP=0, LV_OPA_70=178;
constexpr int LV_ALIGN_TOP_MID=1, LV_ALIGN_TOP_LEFT=2;
constexpr int LV_OBJ_FLAG_SCROLLABLE=1, LV_OBJ_FLAG_CLICKABLE=2, LV_OBJ_FLAG_HIDDEN=4, LV_OBJ_FLAG_PRESS_LOCK=8;
constexpr int LV_RADIUS_CIRCLE=32767;
constexpr int LV_IMG_CF_TRUE_COLOR=1;
constexpr int LV_EVENT_ALL=0, LV_EVENT_PRESSED=1, LV_EVENT_PRESSING=2, LV_EVENT_RELEASED=3, LV_EVENT_PRESS_LOST=4;
inline lv_color_t lv_color_hex(std::uint32_t v){ return {static_cast<std::uint16_t>(v)}; }
inline lv_color_t lv_color_white(){ return {0xFFFF}; }
inline lv_obj_t* lv_scr_act(){ static lv_obj_t o; return &o; }
inline lv_obj_t* lv_canvas_create(lv_obj_t*){ return new lv_obj_t; }
inline lv_obj_t* lv_obj_create(lv_obj_t*){ return new lv_obj_t; }
inline lv_obj_t* lv_line_create(lv_obj_t*){ return new lv_obj_t; }
inline lv_obj_t* lv_label_create(lv_obj_t*){ return new lv_obj_t; }
inline void lv_obj_del(lv_obj_t* o){ delete o; }
inline void lv_obj_set_size(lv_obj_t*, lv_coord_t, lv_coord_t){}
inline void lv_obj_align(lv_obj_t*, int, lv_coord_t, lv_coord_t){}
inline void lv_obj_set_style_bg_color(lv_obj_t*, lv_color_t, int){}
inline void lv_obj_set_style_bg_opa(lv_obj_t*, int, int){}
inline void lv_obj_set_style_line_color(lv_obj_t*, lv_color_t, int){}
inline void lv_obj_set_style_line_width(lv_obj_t*, int, int){}
inline void lv_obj_set_style_line_rounded(lv_obj_t*, bool, int){}
inline void lv_obj_set_style_radius(lv_obj_t*, int, int){}
inline void lv_obj_set_style_border_color(lv_obj_t*, lv_color_t, int){}
inline void lv_obj_set_style_border_width(lv_obj_t*, int, int){}
inline void lv_obj_set_style_text_font(lv_obj_t*, const lv_font_t*, int){}
inline void lv_obj_set_style_text_color(lv_obj_t*, lv_color_t, int){}
inline void lv_obj_set_style_pad_left(lv_obj_t*, int, int){}
inline void lv_obj_set_style_pad_right(lv_obj_t*, int, int){}
inline void lv_obj_set_style_pad_top(lv_obj_t*, int, int){}
inline void lv_obj_set_style_pad_bottom(lv_obj_t*, int, int){}
inline void lv_obj_remove_style_all(lv_obj_t*){}
inline void lv_obj_clear_flag(lv_obj_t*, int){}
inline void lv_obj_add_flag(lv_obj_t*, int){}
inline void lv_obj_set_pos(lv_obj_t*, lv_coord_t, lv_coord_t){}
inline void lv_obj_set_width(lv_obj_t*, lv_coord_t){}
inline void lv_obj_move_foreground(lv_obj_t*){}
inline void lv_obj_invalidate(lv_obj_t*){}
inline lv_event_code_t lv_event_get_code(lv_event_t*){ return LV_EVENT_ALL; }
inline lv_indev_t* lv_indev_get_act(){ static lv_indev_t i; return &i; }
inline void lv_indev_get_point(lv_indev_t*, lv_point_t* p){ if(p){ p->x=0; p->y=0; } }
using lv_event_cb_t = void (*)(lv_event_t*);
inline void lv_obj_add_event_cb(lv_obj_t*, lv_event_cb_t, int, void*){}
inline void lv_canvas_set_buffer(lv_obj_t*, void*, lv_coord_t, lv_coord_t, int){}
inline void lv_line_set_points(lv_obj_t*, const lv_point_t*, std::uint16_t){}
inline void lv_label_set_long_mode(lv_obj_t*, int){}
inline void lv_label_set_text(lv_obj_t*, const char*){}
inline void lv_label_set_text_fmt(lv_obj_t*, const char*, ...){}
constexpr int LV_LABEL_LONG_DOT=1;
