#include "pages/i2c_detector/i2c_detector_view.hpp"
#include "app_identity.h"

#include <cm0/app.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <utility>

namespace {

class FixtureScanner final : public I2cScanner {
public:
    FixtureScanner()
    {
        const auto *result = std::getenv("I2C_TEST_SCAN_RESULT");
        if (result && std::strcmp(result, "no-buses") == 0) outcome = I2cScanState::no_buses;
        if (result && std::strcmp(result, "error") == 0) outcome = I2cScanState::error;
    }

    bool start() override
    {
        ++starts;
        pending_ = true;
        deliver_result = false;
        return true;
    }

    bool running() const noexcept override { return pending_; }

    bool take_result(I2cScanReport &report) override
    {
        if (!pending_ || !deliver_result) return false;
        pending_ = false;
        if (outcome == I2cScanState::complete) {
            I2cBusScanResult bus;
            bus.bus_number = 1;
            bus.detected[0x3c] = true;
            report.buses.push_back(bus);
        } else if (outcome == I2cScanState::error) {
            report.error = "Unable to scan I2C buses";
        }
        return true;
    }

    unsigned starts = 0;
    bool deliver_result = false;
    I2cScanState outcome = I2cScanState::complete;

private:
    bool pending_ = false;
};

struct TestState {
    cm0_app_context_t context{};
    FixtureScanner *scanner = nullptr;
    std::unique_ptr<I2cDetectorViewModel> model;
    std::unique_ptr<I2cDetectorView> view;
    lv_indev_t *keyboard = nullptr;
    lv_group_t *group = nullptr;
    uint32_t key = LV_KEY_NEXT;
    lv_indev_state_t key_state = LV_INDEV_STATE_RELEASED;
    int32_t bottom_scroll = 0;
    int32_t right_scroll = 0;
    bool completed = false;
} state;

// Let AppKit remove disabled controls, recollect focus targets, and finish
// automatic scroll animations before checking each asynchronous phase.
constexpr uint32_t settle_ms = 600;

lv_obj_t *find(const char *name)
{
    auto *object = lv_obj_find_by_name(lv_screen_active(), name);
    assert(object);
    return object;
}

int focused_address()
{
    auto *focused = lv_group_get_focused(state.group);
    for (int address = 0; address < 128; ++address) {
        char name[24];
        std::snprintf(name, sizeof(name), "i2c_address_%02x", address);
        if (find(name) == focused) return address;
    }
    return -1;
}

void press(uint32_t key)
{
    state.key = key;
    state.key_state = LV_INDEV_STATE_PRESSED;
    lv_indev_read(state.keyboard);
    state.key_state = LV_INDEV_STATE_RELEASED;
    lv_indev_read(state.keyboard);
    lv_obj_update_layout(lv_screen_active());
}

void assert_visible_vertically(lv_obj_t *object, lv_obj_t *viewport)
{
    lv_area_t object_area, viewport_area;
    lv_obj_get_coords(object, &object_area);
    lv_obj_get_content_coords(viewport, &viewport_area);
    assert(object_area.y1 >= viewport_area.y1);
    assert(object_area.y2 <= viewport_area.y2);
}

void finish_navigation(lv_timer_t *timer)
{
    lv_timer_delete(timer);
    auto *scan = find("i2c_scan");
    auto *selector = find("i2c_bus_selector");
    auto *body = find("i2c_body");
    assert(state.model->state() == state.scanner->outcome);
    assert(focused_address() == 0x7f);
    assert(lv_obj_get_scroll_y(body) == state.bottom_scroll);
    assert(lv_obj_get_scroll_x(find("i2c_address_scroller")) == state.right_scroll);
    assert_visible_vertically(find("i2c_address_7f"), body);

    // Tab leaves the final address and returns to the first enabled toolbar
    // control. Shift+Tab returns to the same address in the group order.
    press(LV_KEY_NEXT);
    assert(lv_group_get_focused(state.group) == (state.model->bus_count() ? selector : scan));
    press(LV_KEY_PREV);
    assert(focused_address() == 0x7f);
    for (int row = 6; row >= 0; --row) {
        press(LV_KEY_UP);
        assert(focused_address() == row * 16 + 15);
    }
    press(LV_KEY_UP);
    auto *focused = lv_group_get_focused(state.group);
    assert(focused == scan || focused == selector);
    assert_visible_vertically(focused, body);
    assert(lv_obj_get_scroll_y(body) < state.bottom_scroll);
    assert(state.scanner->starts == 2);

    state.completed = true;
    state.context.request_exit(state.context.user_data);
}

void check_refresh_pending(lv_timer_t *timer)
{
    lv_timer_delete(timer);
    assert(state.model->scanning());
    assert(focused_address() == 0x7f);
    assert(lv_obj_get_scroll_y(find("i2c_body")) > 0);
    assert(lv_obj_get_scroll_x(find("i2c_address_scroller")) == state.right_scroll);
    state.scanner->deliver_result = true;
    lv_timer_create(finish_navigation, settle_ms, nullptr);
}

void exercise_navigation(lv_timer_t *timer)
{
    lv_timer_delete(timer);
    auto *scan = find("i2c_scan");
    auto *body = find("i2c_body");
    auto *scroller = find("i2c_address_scroller");
    assert(state.scanner->starts == 1);
    assert(state.model->state() == state.scanner->outcome);
    assert(lv_obj_get_scroll_y(body) == 0);
    assert(lv_obj_get_scroll_x(scroller) == 0);
    assert(focused_address() == 0);

    // Only after checking untouched startup, begin at the real Scan control.
    // AppKit discovers addresses and chooses the nearest column itself.
    lv_group_focus_obj(scan);
    press(LV_KEY_DOWN);
    const int first = focused_address();
    assert(first >= 0 && first < 16);
    for (int row = 1; row < 8; ++row) {
        press(LV_KEY_DOWN);
        assert(focused_address() == first + row * 16);
    }
    assert(lv_obj_get_scroll_y(body) > 0);
    assert_visible_vertically(lv_group_get_focused(state.group), body);

    // Reach both horizontal edges using actual keypad reads. On a narrow
    // screen this also exercises nested scrolling through the matrix viewport.
    for (int column = first; column > 0; --column) press(LV_KEY_LEFT);
    assert(focused_address() == 0x70);
    assert(lv_obj_get_scroll_x(scroller) == 0);
    for (int column = 1; column < 16; ++column) {
        press(LV_KEY_RIGHT);
        assert(focused_address() == 0x70 + column);
    }
    if (lv_display_get_horizontal_resolution(lv_display_get_default()) < 480) {
        assert(lv_obj_get_scroll_x(scroller) > 0);
    }
    lv_area_t address_area, viewport_area;
    lv_obj_get_coords(find("i2c_address_7f"), &address_area);
    lv_obj_get_content_coords(scroller, &viewport_area);
    assert(address_area.x1 >= viewport_area.x1 && address_area.x2 <= viewport_area.x2);
    assert_visible_vertically(find("i2c_address_7f"), body);

    // Address selection only changes focus; Enter must not launch a scan.
    press(LV_KEY_ENTER);
    assert(state.scanner->starts == 1);
    assert(state.model->state() == state.scanner->outcome);
    assert(state.model->selected_bus_index() == 0);
    assert(focused_address() == 0x7f);

    // A later report refresh must preserve the user's place at the bottom,
    // instead of repairing startup by unconditionally resetting scroll/focus.
    state.bottom_scroll = lv_obj_get_scroll_y(body);
    state.right_scroll = lv_obj_get_scroll_x(scroller);
    assert(state.model->start_scan());
    lv_timer_create(check_refresh_pending, settle_ms, nullptr);
}

void check_startup(lv_timer_t *timer)
{
    lv_timer_delete(timer);
    state.group = lv_group_get_default();
    assert(state.group && lv_indev_get_group(state.keyboard) == state.group);
    assert(state.model->scanning());
    assert(lv_obj_get_scroll_y(find("i2c_body")) == 0);
    assert(lv_obj_get_scroll_x(find("i2c_address_scroller")) == 0);
    assert(focused_address() == 0);
    state.scanner->deliver_result = true;
    lv_timer_create(exercise_navigation, settle_ms, nullptr);
}

void open_test(const cm0_app_context_t *context)
{
    state.context = *context;
    auto scanner = std::make_unique<FixtureScanner>();
    state.scanner = scanner.get();
    state.model = std::make_unique<I2cDetectorViewModel>(std::move(scanner));
    state.view = std::make_unique<I2cDetectorView>(*state.model);
    state.view->create(context->root);

    for (auto *input = lv_indev_get_next(nullptr); input; input = lv_indev_get_next(input)) {
        if (lv_indev_get_type(input) == LV_INDEV_TYPE_KEYPAD) state.keyboard = input;
    }
    assert(state.keyboard);
    // Keep the keyboard, group, and callbacks created by cm0_app_run. Only
    // replace hardware samples with deterministic presses using public LVGL
    // APIs, so the real AppKit collection/navigation path remains under test.
    lv_indev_set_mode(state.keyboard, LV_INDEV_MODE_EVENT);
    lv_indev_set_read_cb(state.keyboard, [](lv_indev_t *, lv_indev_data_t *data) {
        data->key = state.key;
        data->state = state.key_state;
    });
    lv_timer_create(check_startup, settle_ms, nullptr);
}

void close_test()
{
    state.view->destroy();
    state.view.reset();
    state.model.reset();
}

}  // namespace

int main(int argc, char **argv)
{
    const cm0_app_descriptor_t descriptor{
        CM0_APP_API_VERSION, sizeof(cm0_app_descriptor_t), LILYGO_UI_I2CDETECTOR_APP_ID, open_test, close_test};
    const int result = cm0_app_run(argc, argv, &descriptor);
    assert(result == 0 && state.completed);
    return 0;
}
