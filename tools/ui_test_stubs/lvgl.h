#pragma once
#include <cstdint>
#include <cstdarg>
struct lv_obj_t {};
struct lv_event_t {};
struct lv_font_t {};
using lv_coord_t = std::int16_t;
using lv_event_code_t = int;
using lv_color_t = std::uint16_t;
using lv_event_cb_t = void (*)(lv_event_t*);
inline lv_font_t lv_font_montserrat_14, lv_font_montserrat_16, lv_font_montserrat_18, lv_font_montserrat_22;
constexpr int LV_EVENT_CLICKED=1, LV_EVENT_READY=2, LV_EVENT_CANCEL=3, LV_EVENT_VALUE_CHANGED=4, LV_EVENT_ALL=5;
constexpr int LV_ALIGN_CENTER=0, LV_ALIGN_TOP_LEFT=1, LV_ALIGN_TOP_RIGHT=2, LV_ALIGN_TOP_MID=3, LV_ALIGN_BOTTOM_MID=4;
constexpr int LV_OPA_COVER=255, LV_OPA_20=51;
constexpr int LV_OBJ_FLAG_CLICKABLE=1;
constexpr int LV_STATE_FOCUSED=1, LV_STATE_DISABLED=2;
constexpr int LV_KEYBOARD_MODE_TEXT_UPPER=0, LV_KEYBOARD_MODE_NUMBER=1;
constexpr int LV_LABEL_LONG_WRAP=1;
constexpr int LV_DIR_VER=2;
constexpr int LV_SCROLLBAR_MODE_AUTO=1;
constexpr int LV_ANIM_OFF=0;
inline lv_event_code_t lv_event_get_code(lv_event_t*) { return 0; }
inline void* lv_event_get_user_data(lv_event_t*) { return nullptr; }
inline lv_obj_t* lv_scr_act() { static lv_obj_t o; return &o; }
inline lv_obj_t* lv_obj_create(lv_obj_t*) { static lv_obj_t o; return &o; }
inline lv_obj_t* lv_label_create(lv_obj_t*) { static lv_obj_t o; return &o; }
inline lv_obj_t* lv_textarea_create(lv_obj_t*) { static lv_obj_t o; return &o; }
inline lv_obj_t* lv_keyboard_create(lv_obj_t*) { static lv_obj_t o; return &o; }
inline lv_obj_t* lv_btn_create(lv_obj_t*) { static lv_obj_t o; return &o; }
inline lv_obj_t* lv_slider_create(lv_obj_t*) { static lv_obj_t o; return &o; }
inline lv_obj_t* lv_dropdown_create(lv_obj_t*) { static lv_obj_t o; return &o; }
inline void lv_obj_remove_style_all(lv_obj_t*) {}
inline void lv_obj_set_size(lv_obj_t*, lv_coord_t, lv_coord_t) {}
inline void lv_obj_set_width(lv_obj_t*, lv_coord_t) {}
inline void lv_obj_align(lv_obj_t*, int, lv_coord_t, lv_coord_t) {}
inline void lv_obj_center(lv_obj_t*) {}
inline void lv_obj_move_foreground(lv_obj_t*) {}
inline void lv_obj_add_flag(lv_obj_t*, int) {}
inline void lv_obj_add_state(lv_obj_t*, int) {}
inline void lv_obj_clear_state(lv_obj_t*, int) {}
inline void lv_obj_del(lv_obj_t*) {}
inline void lv_obj_set_style_bg_color(lv_obj_t*, lv_color_t, int) {}
inline void lv_obj_set_style_bg_opa(lv_obj_t*, int, int) {}
inline void lv_obj_set_style_border_color(lv_obj_t*, lv_color_t, int) {}
inline void lv_obj_set_style_border_width(lv_obj_t*, int, int) {}
inline void lv_obj_set_style_radius(lv_obj_t*, int, int) {}
inline void lv_obj_set_style_pad_all(lv_obj_t*, int, int) {}
inline void lv_obj_set_style_text_font(lv_obj_t*, const lv_font_t*, int) {}
inline void lv_obj_set_style_text_color(lv_obj_t*, lv_color_t, int) {}
inline void lv_obj_set_scroll_dir(lv_obj_t*, int) {}
inline void lv_obj_set_scrollbar_mode(lv_obj_t*, int) {}
inline void lv_obj_add_event_cb(lv_obj_t*, lv_event_cb_t, int, void*) {}
inline lv_color_t lv_color_hex(std::uint32_t v) { return v; }
inline void lv_label_set_text(lv_obj_t*, const char*) {}
inline void lv_label_set_text_fmt(lv_obj_t*, const char*, ...) {}
inline void lv_label_set_long_mode(lv_obj_t*, int) {}
inline void lv_textarea_set_one_line(lv_obj_t*, bool) {}
inline void lv_textarea_set_text(lv_obj_t*, const char*) {}
inline void lv_textarea_set_max_length(lv_obj_t*, std::uint32_t) {}
inline void lv_textarea_set_accepted_chars(lv_obj_t*, const char*) {}
inline const char* lv_textarea_get_text(lv_obj_t*) { return ""; }
inline void lv_keyboard_set_textarea(lv_obj_t*, lv_obj_t*) {}
inline void lv_keyboard_set_mode(lv_obj_t*, int) {}
inline void lv_slider_set_range(lv_obj_t*, std::int32_t, std::int32_t) {}
inline void lv_slider_set_value(lv_obj_t*, std::int32_t, int) {}
inline std::int32_t lv_slider_get_value(lv_obj_t*) { return 70; }
inline void lv_dropdown_set_options(lv_obj_t*, const char*) {}
inline void lv_dropdown_set_selected(lv_obj_t*, std::uint16_t) {}
inline std::uint16_t lv_dropdown_get_selected(lv_obj_t*) { return 2; }

struct lv_chart_series_t {};
constexpr lv_coord_t LV_CHART_POINT_NONE = static_cast<lv_coord_t>(-32768);
constexpr int LV_CHART_TYPE_LINE = 1;
constexpr int LV_CHART_AXIS_PRIMARY_Y = 1;
constexpr int LV_PART_MAIN = 0;
constexpr int LV_PART_INDICATOR = 1;
constexpr int LV_PART_ITEMS = 2;
constexpr int LV_OBJ_FLAG_SCROLLABLE = 2;
inline void lv_obj_clear_flag(lv_obj_t*, int) {}
inline lv_obj_t* lv_chart_create(lv_obj_t*) { static lv_obj_t o; return &o; }
inline void lv_chart_set_type(lv_obj_t*, int) {}
inline void lv_chart_set_point_count(lv_obj_t*, std::uint16_t) {}
inline void lv_chart_set_range(lv_obj_t*, int, lv_coord_t, lv_coord_t) {}
inline void lv_chart_set_div_line_count(lv_obj_t*, std::uint8_t, std::uint8_t) {}
inline lv_chart_series_t* lv_chart_add_series(lv_obj_t*, lv_color_t, int) { static lv_chart_series_t s; return &s; }
inline void lv_chart_set_value_by_id(lv_obj_t*, lv_chart_series_t*, std::uint16_t, lv_coord_t) {}
inline void lv_chart_refresh(lv_obj_t*) {}
inline void lv_obj_set_style_line_color(lv_obj_t*, lv_color_t, int) {}
inline void lv_obj_set_style_line_width(lv_obj_t*, int, int) {}
inline void lv_obj_set_style_size(lv_obj_t*, int, int) {}

constexpr int LV_ALIGN_BOTTOM_LEFT=5, LV_ALIGN_RIGHT_MID=6, LV_ALIGN_LEFT_MID=7;
constexpr int LV_KEYBOARD_MODE_TEXT_LOWER=2;
constexpr int LV_LABEL_LONG_DOT=2;
constexpr int LV_FLEX_FLOW_COLUMN=1;
constexpr int LV_FLEX_ALIGN_START=0, LV_FLEX_ALIGN_CENTER=1;
constexpr int LV_ANIM_ON=1;
inline void lv_obj_set_style_pad_left(lv_obj_t*, int, int) {}
inline void lv_obj_set_style_pad_right(lv_obj_t*, int, int) {}
inline void lv_obj_set_style_pad_top(lv_obj_t*, int, int) {}
inline void lv_obj_set_style_pad_bottom(lv_obj_t*, int, int) {}
inline void lv_obj_set_style_pad_row(lv_obj_t*, int, int) {}
inline void lv_textarea_set_password_mode(lv_obj_t*, bool) {}
inline void lv_textarea_set_password_show_time(lv_obj_t*, std::uint32_t) {}
inline void lv_obj_set_flex_flow(lv_obj_t*, int) {}
inline void lv_obj_set_flex_align(lv_obj_t*, int, int, int) {}
inline void lv_obj_scroll_by(lv_obj_t*, lv_coord_t, lv_coord_t, int) {}
inline void lv_obj_set_height(lv_obj_t*, lv_coord_t) {}
struct lv_point_t { lv_coord_t x = 0; lv_coord_t y = 0; };
struct lv_indev_t {};
constexpr int LV_EVENT_PRESSED=6, LV_EVENT_PRESSING=7, LV_EVENT_RELEASED=8, LV_EVENT_PRESS_LOST=9;
constexpr int LV_OBJ_FLAG_HIDDEN=4, LV_OBJ_FLAG_PRESS_LOCK=8;
constexpr int LV_OPA_TRANSP=0, LV_OPA_70=178;
constexpr int LV_RADIUS_CIRCLE=32767;
constexpr int LV_IMG_CF_TRUE_COLOR=1;
inline lv_indev_t* lv_indev_get_act() { static lv_indev_t i; return &i; }
inline void lv_indev_get_point(lv_indev_t*, lv_point_t* p) { if (p) *p = {}; }
inline void lv_obj_set_pos(lv_obj_t*, lv_coord_t, lv_coord_t) {}
inline lv_obj_t* lv_canvas_create(lv_obj_t*) { static lv_obj_t o; return &o; }
inline void lv_canvas_set_buffer(lv_obj_t*, void*, lv_coord_t, lv_coord_t, int) {}
inline lv_obj_t* lv_line_create(lv_obj_t*) { static lv_obj_t o; return &o; }
inline void lv_line_set_points(lv_obj_t*, const lv_point_t*, std::uint16_t) {}
inline void lv_obj_set_style_line_rounded(lv_obj_t*, bool, int) {}
inline lv_color_t lv_color_white() { return 0xFFFFU; }
inline void lv_obj_invalidate(lv_obj_t*) {}
struct lv_img_dsc_t {};
struct lv_style_t {};
constexpr int LV_TEXT_ALIGN_RIGHT=1;
constexpr int LV_ALIGN_BOTTOM_RIGHT=8;
inline constexpr const char* LV_SYMBOL_GPS="G";
inline constexpr const char* LV_SYMBOL_BELL="B";
inline constexpr const char* LV_SYMBOL_WIFI="W";
inline constexpr const char* LV_SYMBOL_SAVE="S";
inline lv_obj_t* lv_img_create(lv_obj_t*) { static lv_obj_t o; return &o; }
inline void lv_img_set_src(lv_obj_t*, const void*) {}
inline void lv_obj_set_style_img_recolor(lv_obj_t*, lv_color_t, int) {}
inline void lv_obj_set_style_img_recolor_opa(lv_obj_t*, int, int) {}
inline void lv_obj_remove_style(lv_obj_t*, lv_style_t*, int) {}
inline void lv_obj_add_style(lv_obj_t*, lv_style_t*, int) {}
inline void lv_obj_scroll_to_view(lv_obj_t*, int) {}
inline void lv_obj_set_style_text_align(lv_obj_t*, int, int) {}
inline void lv_obj_clean(lv_obj_t*) {}
inline lv_font_t lv_font_montserrat_28;
inline lv_obj_t* lv_bar_create(lv_obj_t*) { static lv_obj_t o; return &o; }
inline void lv_bar_set_range(lv_obj_t*, std::int32_t, std::int32_t) {}
inline void lv_bar_set_value(lv_obj_t*, std::int32_t, int) {}
inline lv_obj_t* lv_obj_get_parent(lv_obj_t*) { static lv_obj_t o; return &o; }
constexpr int LV_TEXT_ALIGN_CENTER=0;
using lv_align_t = int;
inline void lv_obj_scroll_to_y(lv_obj_t*, lv_coord_t, int) {}
using lv_style_selector_t = std::uint32_t;
constexpr int LV_STATE_DEFAULT=0, LV_STATE_PRESSED=4;
constexpr int LV_BORDER_SIDE_BOTTOM=1;
constexpr int LV_FLEX_FLOW_ROW=2;
constexpr int LV_FLEX_ALIGN_SPACE_BETWEEN=2;
inline constexpr const char* LV_SYMBOL_BATTERY_EMPTY="b0";
inline constexpr const char* LV_SYMBOL_CHARGE="c";
inline constexpr const char* LV_SYMBOL_USB="u";
inline constexpr const char* LV_SYMBOL_BATTERY_FULL="b4";
inline constexpr const char* LV_SYMBOL_BATTERY_3="b3";
inline constexpr const char* LV_SYMBOL_BATTERY_2="b2";
inline constexpr const char* LV_SYMBOL_BATTERY_1="b1";
inline constexpr const char* LV_SYMBOL_UP="^";
inline constexpr const char* LV_SYMBOL_DOWN="v";
inline constexpr const char* LV_SYMBOL_OK="o";
inline constexpr const char* LV_SYMBOL_LEFT="<";
inline void lv_obj_set_style_border_side(lv_obj_t*, int, int) {}
inline void lv_obj_set_style_pad_column(lv_obj_t*, int, int) {}
#undef LV_SYMBOL_OK
#define LV_SYMBOL_OK "o"
