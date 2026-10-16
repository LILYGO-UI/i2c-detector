#include "components/components.hpp"
#include "pages/i2c_detector/i2c_detector_view.hpp"

#include <cm0/typography.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr int kPartialRows = 80;

std::vector<lv_color32_t> framebuffer;
int display_width = 0;

void flush_display(lv_display_t *display, const lv_area_t *area, std::uint8_t *pixels)
{
    const int width    = area->x2 - area->x1 + 1;
    const auto *source = reinterpret_cast<const lv_color32_t *>(pixels);

    for (int y = area->y1; y <= area->y2; ++y) {
        const auto source_offset = static_cast<std::size_t>(y - area->y1) * static_cast<std::size_t>(width);
        const auto destination_offset =
            static_cast<std::size_t>(y) * static_cast<std::size_t>(display_width) + static_cast<std::size_t>(area->x1);
        std::copy_n(source + source_offset, width, framebuffer.begin() + destination_offset);
    }
    lv_display_flush_ready(display);
}

bool write_ppm(const char *path, int width, int height)
{
    std::ofstream file(path, std::ios::binary);
    if (!file) return false;

    file << "P6\n" << width << ' ' << height << "\n255\n";
    for (const auto &pixel : framebuffer) {
        file.put(static_cast<char>(pixel.red));
        file.put(static_cast<char>(pixel.green));
        file.put(static_cast<char>(pixel.blue));
    }
    return file.good();
}

bool colors_differ(const lv_color32_t &left, const lv_color32_t &right)
{
    return left.red != right.red || left.green != right.green || left.blue != right.blue || left.alpha != right.alpha;
}

I2cScanReport fixture_report()
{
    I2cScanReport report;
    I2cBusScanResult bus_one;
    bus_one.bus_number     = 1;
    bus_one.detected[0x20] = true;
    report.buses.push_back(bus_one);

    I2cBusScanResult bus_four;
    bus_four.bus_number     = 4;
    bus_four.detected[0x3c] = true;
    bus_four.detected[0x68] = true;
    report.buses.push_back(bus_four);
    return report;
}

class FixtureScanner final : public I2cScanner {
public:
    bool start() override
    {
        pending_ = fixture_report();
        return true;
    }

    [[nodiscard]] bool running() const noexcept override
    {
        return false;
    }

    bool take_result(I2cScanReport &report) override
    {
        if (pending_.buses.empty()) return false;
        report = std::move(pending_);
        return true;
    }

private:
    I2cScanReport pending_;
};

}  // namespace

int main(int argc, char **argv)
{
    assert(argc == 4);
    const int width  = std::atoi(argv[1]);
    const int height = std::atoi(argv[2]);
    assert(width >= 320 && height >= 240);

    display_width = width;
    framebuffer.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));
    std::vector<lv_color32_t> draw_buffer(static_cast<std::size_t>(width) * kPartialRows);

    lv_init();
    auto *display = lv_display_create(width, height);
    assert(display);
    lv_display_set_buffers(display, draw_buffer.data(), nullptr,
                           static_cast<std::uint32_t>(draw_buffer.size() * sizeof(lv_color32_t)),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush_display);
    lv_display_set_default(display);

    I2cDetectorViewModel view_model(std::make_unique<FixtureScanner>());
    I2cDetectorView view(view_model);
    view.create(lv_screen_active());
    lv_tick_inc(120);
    lv_timer_handler();
    lv_obj_update_layout(lv_screen_active());

    auto *surface    = lv_obj_find_by_name(lv_screen_active(), "app_surface");
    auto *controls   = lv_obj_find_by_name(lv_screen_active(), "i2c_controls");
    auto *selector   = lv_obj_find_by_name(lv_screen_active(), "i2c_bus_selector");
    auto *scan       = lv_obj_find_by_name(lv_screen_active(), "i2c_scan");
    auto *status     = lv_obj_find_by_name(lv_screen_active(), "i2c_status");
    auto *panel      = lv_obj_find_by_name(lv_screen_active(), "i2c_address_panel");
    auto *summary    = lv_obj_find_by_name(lv_screen_active(), "i2c_bus_summary");
    auto *address_20 = lv_obj_find_by_name(lv_screen_active(), "i2c_address_20");
    auto *address_3c = lv_obj_find_by_name(lv_screen_active(), "i2c_address_3c");
    auto *address_68 = lv_obj_find_by_name(lv_screen_active(), "i2c_address_68");
    auto *address_7f = lv_obj_find_by_name(lv_screen_active(), "i2c_address_7f");
    assert(surface && controls && selector && scan && status && panel && summary);
    assert(address_20 && address_3c && address_68 && address_7f);
    assert(
        lv_color_eq(lv_obj_get_style_bg_color(surface, LV_PART_MAIN), lv_color_hex(components::color_page_background)));
    assert(lv_color_eq(lv_obj_get_style_bg_color(panel, LV_PART_MAIN),
                       lv_color_hex(components::color_container_background)));
    assert(std::string(lv_label_get_text(status)) == "3 devices found on 2 buses");
    assert(std::string(lv_dropdown_get_options(selector)) == "/dev/i2c-1\n/dev/i2c-4");
    assert(lv_color_eq(lv_obj_get_style_bg_color(address_20, LV_PART_MAIN), lv_color_hex(components::color_detected)));
    assert(!lv_color_eq(lv_obj_get_style_bg_color(address_3c, LV_PART_MAIN), lv_color_hex(components::color_detected)));
    assert(lv_color_eq(lv_obj_get_style_bg_color(address_7f, LV_PART_MAIN),
                       lv_color_hex(components::color_muted_surface)));

    lv_dropdown_set_selected(selector, 1);
    lv_obj_send_event(selector, LV_EVENT_VALUE_CHANGED, nullptr);
    assert(std::string(lv_label_get_text(summary)) == "2 devices detected");
    assert(lv_color_eq(lv_obj_get_style_bg_color(address_3c, LV_PART_MAIN), lv_color_hex(components::color_detected)));
    assert(lv_color_eq(lv_obj_get_style_bg_color(address_68, LV_PART_MAIN), lv_color_hex(components::color_detected)));

    lv_obj_send_event(scan, LV_EVENT_CLICKED, nullptr);
    assert(lv_obj_has_state(scan, LV_STATE_DISABLED));
    lv_tick_inc(120);
    lv_timer_handler();
    assert(!lv_obj_has_state(scan, LV_STATE_DISABLED));

    lv_refr_now(display);
    const bool has_contrast = std::any_of(framebuffer.cbegin() + 1, framebuffer.cend(),
                                          [](const auto &pixel) { return colors_differ(pixel, framebuffer[0]); });
    assert(has_contrast);
    assert(write_ppm(argv[3], width, height));

    lv_dropdown_open(selector);
    lv_refr_now(display);
    const auto dropdown_path = std::string(argv[3]) + ".dropdown.ppm";
    assert(write_ppm(dropdown_path.c_str(), width, height));
    lv_dropdown_close(selector);

    view.destroy();
    lv_display_delete(display);
    lilygo_ui_fonts_deinit();
    lv_deinit();
    return 0;
}
