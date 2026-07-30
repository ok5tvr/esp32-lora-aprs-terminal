# Validation 1.2.2

## Build fix

- `gps_screen.cpp` references only fonts enabled in `include/lv_conf.h`.
- The latest NMEA sentence remains a single-line `LV_LABEL_LONG_DOT` label.
- Navigation style selectors combine LVGL part/state values after conversion to `std::uint32_t`.
- No `lv_font_montserrat_12` references remain in project sources.
