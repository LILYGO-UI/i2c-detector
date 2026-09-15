#include "pages/i2c_detector/i2c_detector_view_model.hpp"

#include <cassert>
#include <memory>
#include <string>
#include <utility>

namespace {

class FakeScanner final : public I2cScanner {
public:
    bool start() override
    {
        if (running_) return false;
        running_ = true;
        return true;
    }

    [[nodiscard]] bool running() const noexcept override
    {
        return running_;
    }

    bool take_result(I2cScanReport &report) override
    {
        if (!ready_) return false;
        report = std::move(report_);
        ready_ = false;
        return true;
    }

    void complete(I2cScanReport report)
    {
        report_  = std::move(report);
        running_ = false;
        ready_   = true;
    }

private:
    bool running_ = false;
    bool ready_   = false;
    I2cScanReport report_;
};

I2cScanReport detected_report()
{
    I2cScanReport report;
    I2cBusScanResult bus_four;
    bus_four.bus_number     = 4;
    bus_four.detected[0x3c] = true;
    bus_four.detected[0x68] = true;
    report.buses.push_back(bus_four);

    I2cBusScanResult bus_one;
    bus_one.bus_number     = 1;
    bus_one.detected[0x20] = true;
    report.buses.push_back(bus_one);
    return report;
}

}  // namespace

int main()
{
    lv_init();

    {
        auto scanner      = std::make_unique<FakeScanner>();
        auto *scanner_ptr = scanner.get();
        I2cDetectorViewModel view_model(std::move(scanner));
        assert(view_model.state() == I2cScanState::ready);
        assert(lv_subject_get_int(view_model.revision_subject()) == 0);

        assert(view_model.start_scan());
        assert(view_model.scanning());
        assert(!view_model.start_scan());
        assert(lv_subject_get_int(view_model.revision_subject()) == 1);

        scanner_ptr->complete(detected_report());
        assert(view_model.poll());
        assert(!view_model.poll());
        assert(view_model.state() == I2cScanState::complete);
        assert(view_model.bus_count() == 2);
        assert(view_model.selected_bus()->bus_number == 1);
        assert(view_model.detected(0x20));
        assert(view_model.bus_options() == "/dev/i2c-1\n/dev/i2c-4");
        assert(view_model.status_text() == "3 devices found on 2 buses");

        assert(view_model.select_bus(1));
        assert(view_model.selected_bus_title() == "/dev/i2c-4");
        assert(view_model.selected_bus_summary() == "2 devices detected");
        assert(view_model.detected(0x3c));
        assert(view_model.detected(0x68));
        assert(!view_model.detected(0x69));
        assert(lv_subject_get_int(view_model.revision_subject()) == 3);
    }

    lv_deinit();
    return 0;
}
