#include "pages/i2c_detector/i2c_detector_model.hpp"

#include <cassert>
#include <initializer_list>
#include <string>
#include <utility>

namespace {

I2cBusScanResult bus_result(int bus_number, std::initializer_list<int> addresses, std::string error = {})
{
    I2cBusScanResult result;
    result.bus_number = bus_number;
    result.error      = std::move(error);
    for (const int address : addresses) result.detected[static_cast<std::size_t>(address)] = true;
    return result;
}

void test_scan_lifecycle_and_selection()
{
    I2cDetectorModel model;
    assert(model.state() == I2cScanState::ready);
    assert(model.selected_bus() == nullptr);

    model.begin_scan();
    assert(model.state() == I2cScanState::scanning);

    I2cScanReport report;
    report.buses.push_back(bus_result(4, {0x3c, 0x68}));
    report.buses.push_back(bus_result(1, {0x20}));
    report.buses.push_back(bus_result(7, {}, "Permission denied"));
    model.finish_scan(std::move(report));

    assert(model.state() == I2cScanState::complete);
    assert(model.buses().size() == 3);
    assert(model.buses()[0].bus_number == 1);
    assert(model.total_device_count() == 3);
    assert(model.unavailable_bus_count() == 1);
    assert(model.selected_bus()->bus_number == 1);
    assert(model.select_bus(1));
    assert(model.selected_bus()->bus_number == 4);
    assert(model.selected_bus()->device_count() == 2);
    assert(!model.select_bus(3));

    model.begin_scan();
    I2cScanReport rescanned;
    rescanned.buses.push_back(bus_result(5, {0x55}));
    rescanned.buses.push_back(bus_result(4, {0x3c}));
    model.finish_scan(std::move(rescanned));
    assert(model.selected_bus()->bus_number == 4);
}

void test_empty_and_error_reports()
{
    I2cDetectorModel model;
    model.finish_scan({});
    assert(model.state() == I2cScanState::no_buses);

    I2cScanReport failed;
    failed.error = "Cannot inspect /dev";
    model.finish_scan(std::move(failed));
    assert(model.state() == I2cScanState::error);
    assert(model.error() == "Cannot inspect /dev");
}

}  // namespace

int main()
{
    test_scan_lifecycle_and_selection();
    test_empty_and_error_reports();
    return 0;
}
