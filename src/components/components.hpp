#ifndef LILYGO_UI_I2CDETECTOR_COMPONENTS_HPP
#define LILYGO_UI_I2CDETECTOR_COMPONENTS_HPP

#include <lvgl.h>

#include <cstdint>

namespace components {

inline constexpr std::uint32_t color_page_background      = 0xf2f2f7;
inline constexpr std::uint32_t color_container_background = 0xffffff;
inline constexpr std::uint32_t color_container_title      = 0x1a1a1a;
inline constexpr std::uint32_t color_text                 = 0x000000;
inline constexpr std::uint32_t color_border               = 0xd8dde3;
inline constexpr std::uint32_t color_action               = 0x20262d;
inline constexpr std::uint32_t color_detected             = 0x167c5a;
inline constexpr std::uint32_t color_detected_border      = 0x0f684b;
inline constexpr std::uint32_t color_muted_surface        = 0xe9edf1;
inline constexpr std::uint32_t color_muted_text           = 0x68727d;
inline constexpr std::uint32_t color_disabled_text        = 0x9aa1a9;
inline constexpr std::uint32_t color_scanning             = 0xa45a00;
inline constexpr std::uint32_t color_error                = 0xb42318;

lv_obj_t *create_page(lv_obj_t *parent);
lv_obj_t *create_header(lv_obj_t *parent, const char *title);
lv_obj_t *create_card(lv_obj_t *parent);
lv_obj_t *create_button(lv_obj_t *parent, const char *text, lv_event_cb_t callback, void *user_data);

}  // namespace components

#endif
