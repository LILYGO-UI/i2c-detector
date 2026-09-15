#ifndef LILYGO_UI_I2CDETECTOR_PAGES_I2C_SCANNER_HPP
#define LILYGO_UI_I2CDETECTOR_PAGES_I2C_SCANNER_HPP

#include "pages/i2c_detector/i2c_detector_model.hpp"

#include <memory>

class I2cScanner {
public:
    virtual ~I2cScanner() = default;

    virtual bool start()                                = 0;
    [[nodiscard]] virtual bool running() const noexcept = 0;
    virtual bool take_result(I2cScanReport &report)     = 0;
};

std::unique_ptr<I2cScanner> make_system_i2c_scanner();

#endif
