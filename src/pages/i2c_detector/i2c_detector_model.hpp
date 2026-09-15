#ifndef LILYGO_UI_I2CDETECTOR_PAGES_I2C_DETECTOR_MODEL_HPP
#define LILYGO_UI_I2CDETECTOR_PAGES_I2C_DETECTOR_MODEL_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

inline constexpr std::uint8_t i2c_first_probe_address = 0x03;
inline constexpr std::uint8_t i2c_last_probe_address  = 0x77;
inline constexpr std::size_t i2c_address_count        = 0x80;

struct I2cBusScanResult {
    int bus_number = -1;
    std::array<bool, i2c_address_count> detected{};
    std::string error;

    [[nodiscard]] std::size_t device_count() const noexcept;
};

struct I2cScanReport {
    std::vector<I2cBusScanResult> buses;
    std::string error;
};

enum class I2cScanState { ready, scanning, complete, no_buses, error };

class I2cDetectorModel {
public:
    [[nodiscard]] I2cScanState state() const noexcept;
    [[nodiscard]] const std::vector<I2cBusScanResult> &buses() const noexcept;
    [[nodiscard]] const I2cBusScanResult *selected_bus() const noexcept;
    [[nodiscard]] std::size_t selected_bus_index() const noexcept;
    [[nodiscard]] std::size_t total_device_count() const noexcept;
    [[nodiscard]] std::size_t unavailable_bus_count() const noexcept;
    [[nodiscard]] const std::string &error() const noexcept;

    void begin_scan() noexcept;
    void finish_scan(I2cScanReport report);
    bool select_bus(std::size_t index) noexcept;

private:
    I2cScanState state_ = I2cScanState::ready;
    std::vector<I2cBusScanResult> buses_;
    std::size_t selected_bus_index_ = 0;
    std::string error_;
};

#endif
