#ifndef LILYGO_UI_I2CDETECTOR_PAGES_I2C_DETECTOR_VIEW_MODEL_HPP
#define LILYGO_UI_I2CDETECTOR_PAGES_I2C_DETECTOR_VIEW_MODEL_HPP

#include "components/lv_subject.hpp"
#include "pages/i2c_detector/i2c_detector_model.hpp"
#include "pages/i2c_detector/i2c_scanner.hpp"

#include <cstddef>
#include <memory>
#include <string>

class I2cDetectorViewModel {
public:
    explicit I2cDetectorViewModel(std::unique_ptr<I2cScanner> scanner = make_system_i2c_scanner());

    [[nodiscard]] I2cScanState state() const noexcept;
    [[nodiscard]] bool scanning() const noexcept;
    [[nodiscard]] std::size_t bus_count() const noexcept;
    [[nodiscard]] std::size_t selected_bus_index() const noexcept;
    [[nodiscard]] const I2cBusScanResult *selected_bus() const noexcept;
    [[nodiscard]] bool detected(std::uint8_t address) const noexcept;
    [[nodiscard]] std::string bus_options() const;
    [[nodiscard]] std::string status_text() const;
    [[nodiscard]] std::string selected_bus_title() const;
    [[nodiscard]] std::string selected_bus_summary() const;
    [[nodiscard]] lv_subject_t *revision_subject() noexcept;

    bool start_scan();
    bool poll();
    bool select_bus(std::size_t index) noexcept;

private:
    void publish() noexcept;

    I2cDetectorModel model_;
    std::unique_ptr<I2cScanner> scanner_;
    IntSubject revision_{0};
};

#endif
