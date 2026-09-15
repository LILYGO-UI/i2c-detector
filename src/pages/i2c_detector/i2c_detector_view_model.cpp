#include "pages/i2c_detector/i2c_detector_view_model.hpp"

#include <sstream>
#include <stdexcept>
#include <utility>

I2cDetectorViewModel::I2cDetectorViewModel(std::unique_ptr<I2cScanner> scanner) : scanner_(std::move(scanner))
{
    if (!scanner_) throw std::invalid_argument("I2C scanner must not be null");
}

I2cScanState I2cDetectorViewModel::state() const noexcept
{
    return model_.state();
}

bool I2cDetectorViewModel::scanning() const noexcept
{
    return model_.state() == I2cScanState::scanning;
}

std::size_t I2cDetectorViewModel::bus_count() const noexcept
{
    return model_.buses().size();
}

std::size_t I2cDetectorViewModel::selected_bus_index() const noexcept
{
    return model_.selected_bus_index();
}

const I2cBusScanResult *I2cDetectorViewModel::selected_bus() const noexcept
{
    return model_.selected_bus();
}

bool I2cDetectorViewModel::detected(std::uint8_t address) const noexcept
{
    const auto *bus = model_.selected_bus();
    return bus && bus->detected[address];
}

std::string I2cDetectorViewModel::bus_options() const
{
    if (model_.buses().empty()) return "No I2C buses";

    std::ostringstream options;
    for (std::size_t index = 0; index < model_.buses().size(); ++index) {
        if (index > 0) options << '\n';
        options << "/dev/i2c-" << model_.buses()[index].bus_number;
    }
    return options.str();
}

std::string I2cDetectorViewModel::status_text() const
{
    switch (model_.state()) {
        case I2cScanState::ready:
            return "Ready to scan";
        case I2cScanState::scanning:
            return "Scanning available I2C buses...";
        case I2cScanState::no_buses:
            return "No I2C buses found";
        case I2cScanState::error:
            return model_.error();
        case I2cScanState::complete:
            break;
    }

    std::ostringstream status;
    status << model_.total_device_count() << " device" << (model_.total_device_count() == 1 ? "" : "s") << " found on "
           << model_.buses().size() << " bus" << (model_.buses().size() == 1 ? "" : "es");
    if (model_.unavailable_bus_count() > 0) {
        status << "; " << model_.unavailable_bus_count() << " unavailable";
    }
    return status.str();
}

std::string I2cDetectorViewModel::selected_bus_title() const
{
    const auto *bus = model_.selected_bus();
    return bus ? "/dev/i2c-" + std::to_string(bus->bus_number) : "7-bit address map";
}

std::string I2cDetectorViewModel::selected_bus_summary() const
{
    const auto *bus = model_.selected_bus();
    if (!bus) return "Hexadecimal addresses 00-7F";
    if (!bus->error.empty()) return "Bus unavailable: " + bus->error;

    const auto count = bus->device_count();
    return std::to_string(count) + " device" + (count == 1 ? "" : "s") + " detected";
}

lv_subject_t *I2cDetectorViewModel::revision_subject() noexcept
{
    return revision_.get();
}

bool I2cDetectorViewModel::start_scan()
{
    if (scanner_->running()) return false;

    try {
        if (!scanner_->start()) return false;
        model_.begin_scan();
    } catch (const std::exception &error) {
        I2cScanReport report;
        report.error = error.what();
        model_.finish_scan(std::move(report));
    } catch (...) {
        I2cScanReport report;
        report.error = "Unable to start the I2C scan";
        model_.finish_scan(std::move(report));
    }
    publish();
    return model_.state() == I2cScanState::scanning;
}

bool I2cDetectorViewModel::poll()
{
    I2cScanReport report;
    if (!scanner_->take_result(report)) return false;
    model_.finish_scan(std::move(report));
    publish();
    return true;
}

bool I2cDetectorViewModel::select_bus(std::size_t index) noexcept
{
    if (!model_.select_bus(index)) return false;
    publish();
    return true;
}

void I2cDetectorViewModel::publish() noexcept
{
    revision_.set(revision_.value() + 1);
}
