#include "pages/i2c_detector/i2c_detector_model.hpp"

#include <algorithm>
#include <utility>

std::size_t I2cBusScanResult::device_count() const noexcept
{
    return static_cast<std::size_t>(std::count(detected.cbegin(), detected.cend(), true));
}

I2cScanState I2cDetectorModel::state() const noexcept
{
    return state_;
}

const std::vector<I2cBusScanResult> &I2cDetectorModel::buses() const noexcept
{
    return buses_;
}

const I2cBusScanResult *I2cDetectorModel::selected_bus() const noexcept
{
    if (buses_.empty() || selected_bus_index_ >= buses_.size()) return nullptr;
    return &buses_[selected_bus_index_];
}

std::size_t I2cDetectorModel::selected_bus_index() const noexcept
{
    return selected_bus_index_;
}

std::size_t I2cDetectorModel::total_device_count() const noexcept
{
    std::size_t count = 0;
    for (const auto &bus : buses_) count += bus.device_count();
    return count;
}

std::size_t I2cDetectorModel::unavailable_bus_count() const noexcept
{
    return static_cast<std::size_t>(
        std::count_if(buses_.cbegin(), buses_.cend(), [](const auto &bus) { return !bus.error.empty(); }));
}

const std::string &I2cDetectorModel::error() const noexcept
{
    return error_;
}

void I2cDetectorModel::begin_scan() noexcept
{
    state_ = I2cScanState::scanning;
    error_.clear();
}

void I2cDetectorModel::finish_scan(I2cScanReport report)
{
    const auto *previous          = selected_bus();
    const int previous_bus_number = previous ? previous->bus_number : -1;

    std::sort(report.buses.begin(), report.buses.end(),
              [](const auto &left, const auto &right) { return left.bus_number < right.bus_number; });
    buses_ = std::move(report.buses);
    error_ = std::move(report.error);

    selected_bus_index_ = 0;
    if (previous_bus_number >= 0) {
        const auto found = std::find_if(buses_.cbegin(), buses_.cend(), [previous_bus_number](const auto &bus) {
            return bus.bus_number == previous_bus_number;
        });
        if (found != buses_.cend()) {
            selected_bus_index_ = static_cast<std::size_t>(std::distance(buses_.cbegin(), found));
        }
    }

    if (!error_.empty()) {
        state_ = I2cScanState::error;
    } else if (buses_.empty()) {
        state_ = I2cScanState::no_buses;
    } else {
        state_ = I2cScanState::complete;
    }
}

bool I2cDetectorModel::select_bus(std::size_t index) noexcept
{
    if (index >= buses_.size() || index == selected_bus_index_) return false;
    selected_bus_index_ = index;
    return true;
}
