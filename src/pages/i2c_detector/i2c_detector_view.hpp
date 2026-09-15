#ifndef LILYGO_UI_I2CDETECTOR_PAGES_I2C_DETECTOR_VIEW_HPP
#define LILYGO_UI_I2CDETECTOR_PAGES_I2C_DETECTOR_VIEW_HPP

#include "pages/i2c_detector/i2c_detector_view_model.hpp"

#include <lvgl.h>

#include <array>
#include <cstddef>

class I2cDetectorView {
public:
    explicit I2cDetectorView(I2cDetectorViewModel &view_model) noexcept;

    lv_obj_t *create(lv_obj_t *parent, const char *application_name);
    void destroy() noexcept;

private:
    static void scan_event(lv_event_t *event);
    static void bus_changed_event(lv_event_t *event);
    static void size_changed_event(lv_event_t *event);
    static void poll_timer_event(lv_timer_t *timer);
    static void revision_changed_event(lv_observer_t *observer, lv_subject_t *subject);

    void create_address_matrix();
    void refresh();
    void apply_layout() noexcept;
    void style_address(std::size_t address, bool detected) noexcept;

    I2cDetectorViewModel &view_model_;
    lv_obj_t *surface_           = nullptr;
    lv_obj_t *body_              = nullptr;
    lv_obj_t *controls_panel_    = nullptr;
    lv_obj_t *toolbar_           = nullptr;
    lv_obj_t *bus_dropdown_      = nullptr;
    lv_obj_t *scan_button_       = nullptr;
    lv_obj_t *scan_label_        = nullptr;
    lv_obj_t *status_label_      = nullptr;
    lv_obj_t *matrix_panel_      = nullptr;
    lv_obj_t *matrix_scroller_   = nullptr;
    lv_obj_t *matrix_            = nullptr;
    lv_obj_t *bus_title_label_   = nullptr;
    lv_obj_t *bus_summary_label_ = nullptr;
    lv_timer_t *poll_timer_      = nullptr;
    std::array<lv_obj_t *, i2c_address_count> address_cells_{};
};

#endif
