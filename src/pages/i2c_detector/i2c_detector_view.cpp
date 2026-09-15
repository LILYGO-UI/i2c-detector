#include "pages/i2c_detector/i2c_detector_view.hpp"

#include "components/components.hpp"

#include <cm0/typography.h>

#include <cstdio>
#include <string>

namespace {

void make_unframed(lv_obj_t *object)
{
    lv_obj_set_style_border_width(object, 0, 0);
    lv_obj_set_style_radius(object, 0, 0);
    lv_obj_set_style_bg_opa(object, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(object, 0, 0);
}

}  // namespace

I2cDetectorView::I2cDetectorView(I2cDetectorViewModel &view_model) noexcept : view_model_(view_model)
{
}

lv_obj_t *I2cDetectorView::create(lv_obj_t *parent, const char *application_name)
{
    surface_ = components::create_page(parent);
    lv_obj_add_event_cb(surface_, size_changed_event, LV_EVENT_SIZE_CHANGED, this);
    components::create_header(surface_, application_name);

    body_ = lv_obj_create(surface_);
    lv_obj_set_name_static(body_, "i2c_body");
    lv_obj_set_width(body_, LV_PCT(100));
    lv_obj_set_flex_grow(body_, 1);
    make_unframed(body_);
    lv_obj_set_style_pad_row(body_, 16, 0);
    lv_obj_set_flex_flow(body_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(body_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scroll_dir(body_, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(body_, LV_SCROLLBAR_MODE_AUTO);

    controls_panel_ = components::create_card(body_);
    lv_obj_set_name_static(controls_panel_, "i2c_controls");
    lv_obj_set_height(controls_panel_, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_row(controls_panel_, 14, 0);
    lv_obj_set_flex_flow(controls_panel_, LV_FLEX_FLOW_COLUMN);

    toolbar_ = lv_obj_create(controls_panel_);
    make_unframed(toolbar_);
    lv_obj_set_width(toolbar_, LV_PCT(100));
    lv_obj_set_height(toolbar_, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_column(toolbar_, 12, 0);
    lv_obj_set_flex_flow(toolbar_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(toolbar_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(toolbar_, LV_OBJ_FLAG_SCROLLABLE);

    bus_dropdown_ = lv_dropdown_create(toolbar_);
    lv_obj_set_name_static(bus_dropdown_, "i2c_bus_selector");
    lv_obj_set_height(bus_dropdown_, 56);
    lv_obj_set_width(bus_dropdown_, 0);
    lv_obj_set_flex_grow(bus_dropdown_, 1);
    lv_obj_set_style_radius(bus_dropdown_, 8, 0);
    lv_obj_set_style_border_color(bus_dropdown_, lv_color_hex(components::color_border), 0);
    lv_obj_set_style_bg_color(bus_dropdown_, lv_color_hex(components::color_page_background), 0);
    lv_obj_set_style_text_color(bus_dropdown_, lv_color_hex(components::color_text), 0);
    lv_obj_set_style_text_font(bus_dropdown_, lilygo_ui_font_get(22), 0);
    lv_obj_add_event_cb(bus_dropdown_, bus_changed_event, LV_EVENT_VALUE_CHANGED, this);

    scan_button_ = components::create_button(toolbar_, LV_SYMBOL_REFRESH "  Scan", scan_event, this);
    lv_obj_set_name_static(scan_button_, "i2c_scan");
    lv_obj_set_width(scan_button_, 164);
    lv_obj_set_style_opa(scan_button_, LV_OPA_50, LV_STATE_DISABLED);
    scan_label_ = lv_obj_get_child(scan_button_, 0);

    status_label_ = lv_label_create(controls_panel_);
    lv_obj_set_name_static(status_label_, "i2c_status");
    lv_obj_set_width(status_label_, LV_PCT(100));
    lv_label_set_long_mode(status_label_, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(status_label_, lilygo_ui_font_get(18), 0);
    lv_obj_set_style_text_color(status_label_, lv_color_hex(components::color_muted_text), 0);

    matrix_panel_ = components::create_card(body_);
    lv_obj_set_name_static(matrix_panel_, "i2c_address_panel");
    lv_obj_set_height(matrix_panel_, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_row(matrix_panel_, 8, 0);
    lv_obj_set_flex_flow(matrix_panel_, LV_FLEX_FLOW_COLUMN);

    bus_title_label_ = lv_label_create(matrix_panel_);
    lv_obj_set_name_static(bus_title_label_, "i2c_bus_title");
    lv_obj_set_width(bus_title_label_, LV_PCT(100));
    lv_obj_set_style_text_font(bus_title_label_, lilygo_ui_font_get(22), 0);
    lv_obj_set_style_text_color(bus_title_label_, lv_color_hex(components::color_container_title), 0);

    bus_summary_label_ = lv_label_create(matrix_panel_);
    lv_obj_set_name_static(bus_summary_label_, "i2c_bus_summary");
    lv_obj_set_width(bus_summary_label_, LV_PCT(100));
    lv_label_set_long_mode(bus_summary_label_, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(bus_summary_label_, lilygo_ui_font_get(14), 0);
    lv_obj_set_style_text_color(bus_summary_label_, lv_color_hex(components::color_muted_text), 0);

    auto *legend = lv_obj_create(matrix_panel_);
    make_unframed(legend);
    lv_obj_set_size(legend, LV_PCT(100), 24);
    lv_obj_set_style_pad_column(legend, 8, 0);
    lv_obj_set_flex_flow(legend, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(legend, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(legend, LV_OBJ_FLAG_SCROLLABLE);

    auto *swatch = lv_obj_create(legend);
    lv_obj_set_name_static(swatch, "i2c_detected_swatch");
    lv_obj_set_size(swatch, 14, 14);
    lv_obj_set_style_radius(swatch, 3, 0);
    lv_obj_set_style_border_width(swatch, 0, 0);
    lv_obj_set_style_bg_color(swatch, lv_color_hex(components::color_detected), 0);
    lv_obj_clear_flag(swatch, LV_OBJ_FLAG_SCROLLABLE);

    auto *legend_label = lv_label_create(legend);
    lv_label_set_text(legend_label, "Detected");
    lv_obj_set_style_text_font(legend_label, lilygo_ui_font_get(14), 0);
    lv_obj_set_style_text_color(legend_label, lv_color_hex(components::color_muted_text), 0);

    matrix_scroller_ = lv_obj_create(matrix_panel_);
    lv_obj_set_name_static(matrix_scroller_, "i2c_address_scroller");
    lv_obj_set_width(matrix_scroller_, LV_PCT(100));
    lv_obj_set_height(matrix_scroller_, 348);
    make_unframed(matrix_scroller_);
    lv_obj_set_scroll_dir(matrix_scroller_, LV_DIR_HOR);
    lv_obj_set_scrollbar_mode(matrix_scroller_, LV_SCROLLBAR_MODE_AUTO);

    matrix_ = lv_obj_create(matrix_scroller_);
    lv_obj_set_name_static(matrix_, "i2c_address_matrix");
    lv_obj_set_width(matrix_, LV_PCT(100));
    lv_obj_set_height(matrix_, LV_SIZE_CONTENT);
    make_unframed(matrix_);
    lv_obj_set_style_pad_row(matrix_, 4, 0);
    lv_obj_set_flex_flow(matrix_, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(matrix_, LV_OBJ_FLAG_SCROLLABLE);
    create_address_matrix();

    poll_timer_ = lv_timer_create(poll_timer_event, 100, this);
    lv_subject_add_observer_obj(view_model_.revision_subject(), revision_changed_event, surface_, this);
    apply_layout();
    view_model_.start_scan();
    return surface_;
}

void I2cDetectorView::destroy() noexcept
{
    if (poll_timer_) lv_timer_delete(poll_timer_);
    poll_timer_ = nullptr;
    if (surface_) lv_obj_delete(surface_);
    surface_           = nullptr;
    body_              = nullptr;
    controls_panel_    = nullptr;
    toolbar_           = nullptr;
    bus_dropdown_      = nullptr;
    scan_button_       = nullptr;
    scan_label_        = nullptr;
    status_label_      = nullptr;
    matrix_panel_      = nullptr;
    matrix_scroller_   = nullptr;
    matrix_            = nullptr;
    bus_title_label_   = nullptr;
    bus_summary_label_ = nullptr;
    address_cells_.fill(nullptr);
}

void I2cDetectorView::scan_event(lv_event_t *event)
{
    auto *view = static_cast<I2cDetectorView *>(lv_event_get_user_data(event));
    view->view_model_.start_scan();
}

void I2cDetectorView::bus_changed_event(lv_event_t *event)
{
    auto *view = static_cast<I2cDetectorView *>(lv_event_get_user_data(event));
    view->view_model_.select_bus(lv_dropdown_get_selected(view->bus_dropdown_));
}

void I2cDetectorView::size_changed_event(lv_event_t *event)
{
    static_cast<I2cDetectorView *>(lv_event_get_user_data(event))->apply_layout();
}

void I2cDetectorView::poll_timer_event(lv_timer_t *timer)
{
    auto *view = static_cast<I2cDetectorView *>(lv_timer_get_user_data(timer));
    view->view_model_.poll();
}

void I2cDetectorView::revision_changed_event(lv_observer_t *observer, lv_subject_t *)
{
    static_cast<I2cDetectorView *>(lv_observer_get_user_data(observer))->refresh();
}

void I2cDetectorView::create_address_matrix()
{
    for (std::size_t row = 0; row < 8; ++row) {
        auto *row_object = lv_obj_create(matrix_);
        make_unframed(row_object);
        lv_obj_set_size(row_object, LV_PCT(100), 40);
        lv_obj_set_style_pad_column(row_object, 4, 0);
        lv_obj_set_flex_flow(row_object, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row_object, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(row_object, LV_OBJ_FLAG_SCROLLABLE);

        for (std::size_t column = 0; column < 16; ++column) {
            const std::size_t address = row * 16 + column;
            auto *cell                = lv_obj_create(row_object);
            address_cells_[address]   = cell;
            lv_obj_set_size(cell, 0, 40);
            lv_obj_set_flex_grow(cell, 1);
            lv_obj_set_style_radius(cell, 4, 0);
            lv_obj_set_style_border_width(cell, 1, 0);
            lv_obj_set_style_pad_all(cell, 0, 0);
            lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);

            char name[24];
            std::snprintf(name, sizeof(name), "i2c_address_%02zx", address);
            lv_obj_set_name(cell, name);

            char address_text[3];
            std::snprintf(address_text, sizeof(address_text), "%02zX", address);
            auto *label = lv_label_create(cell);
            lv_label_set_text(label, address_text);
            lv_obj_set_style_text_font(label, lilygo_ui_font_get(14), 0);
            lv_obj_center(label);
            style_address(address, false);
        }
    }
}

void I2cDetectorView::refresh()
{
    if (!surface_) return;

    const auto options = view_model_.bus_options();
    lv_dropdown_set_options(bus_dropdown_, options.c_str());
    lv_dropdown_set_selected(bus_dropdown_, static_cast<std::uint32_t>(view_model_.selected_bus_index()));
    if (view_model_.bus_count() == 0) {
        lv_obj_add_state(bus_dropdown_, LV_STATE_DISABLED);
    } else {
        lv_obj_remove_state(bus_dropdown_, LV_STATE_DISABLED);
    }

    const bool scanning = view_model_.scanning();
    if (scanning) {
        lv_obj_add_state(scan_button_, LV_STATE_DISABLED);
        lv_label_set_text(scan_label_, LV_SYMBOL_REFRESH "  Scanning");
    } else {
        lv_obj_remove_state(scan_button_, LV_STATE_DISABLED);
        lv_label_set_text(scan_label_, LV_SYMBOL_REFRESH "  Scan");
    }

    const auto status = view_model_.status_text();
    lv_label_set_text(status_label_, status.c_str());
    std::uint32_t status_color = components::color_muted_text;
    if (view_model_.state() == I2cScanState::scanning) status_color = components::color_scanning;
    if (view_model_.state() == I2cScanState::complete) status_color = components::color_detected;
    if (view_model_.state() == I2cScanState::error) status_color = components::color_error;
    lv_obj_set_style_text_color(status_label_, lv_color_hex(status_color), 0);

    const auto title = view_model_.selected_bus_title();
    lv_label_set_text(bus_title_label_, title.c_str());
    const auto summary = view_model_.selected_bus_summary();
    lv_label_set_text(bus_summary_label_, summary.c_str());
    lv_obj_set_style_text_color(bus_summary_label_,
                                lv_color_hex(view_model_.selected_bus() && !view_model_.selected_bus()->error.empty()
                                                 ? components::color_error
                                                 : components::color_muted_text),
                                0);

    for (std::size_t address = 0; address < address_cells_.size(); ++address) {
        style_address(address, view_model_.detected(static_cast<std::uint8_t>(address)));
    }
}

void I2cDetectorView::apply_layout() noexcept
{
    if (!surface_ || !body_ || !controls_panel_ || !toolbar_ || !matrix_panel_ || !matrix_) return;

    const auto width   = lv_obj_get_content_width(surface_);
    const bool compact = width < 480;
    const bool wide    = width >= 1000;
    lv_obj_set_style_pad_all(body_, compact ? 12 : 20, 0);
    lv_obj_set_style_pad_bottom(body_, compact ? 28 : 40, 0);
    lv_obj_set_width(controls_panel_, wide ? 1000 : LV_PCT(100));
    lv_obj_set_width(matrix_panel_, wide ? 1000 : LV_PCT(100));
    lv_obj_set_style_pad_all(controls_panel_, compact ? 12 : 18, 0);
    lv_obj_set_style_pad_all(matrix_panel_, compact ? 10 : 16, 0);

    if (compact) {
        lv_obj_set_flex_flow(toolbar_, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(toolbar_, 10, 0);
        lv_obj_set_width(bus_dropdown_, LV_PCT(100));
        lv_obj_set_flex_grow(bus_dropdown_, 0);
        lv_obj_set_width(scan_button_, LV_PCT(100));
        lv_obj_set_width(matrix_, 500);
    } else {
        lv_obj_set_flex_flow(toolbar_, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_column(toolbar_, 12, 0);
        lv_obj_set_width(bus_dropdown_, 0);
        lv_obj_set_flex_grow(bus_dropdown_, 1);
        lv_obj_set_width(scan_button_, 164);
        lv_obj_set_width(matrix_, LV_PCT(100));
    }
}

void I2cDetectorView::style_address(std::size_t address, bool detected) noexcept
{
    auto *cell = address_cells_[address];
    if (!cell) return;

    const bool probeable  = address >= i2c_first_probe_address && address <= i2c_last_probe_address;
    const auto background = detected    ? components::color_detected
                            : probeable ? components::color_container_background
                                        : components::color_muted_surface;
    const auto foreground = detected    ? components::color_container_background
                            : probeable ? components::color_text
                                        : components::color_disabled_text;
    const auto border     = detected ? components::color_detected_border : components::color_border;
    lv_obj_set_style_bg_color(cell, lv_color_hex(background), 0);
    lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(cell, lv_color_hex(border), 0);
    if (lv_obj_get_child_count(cell) > 0) {
        lv_obj_set_style_text_color(lv_obj_get_child(cell, 0), lv_color_hex(foreground), 0);
    }
}
