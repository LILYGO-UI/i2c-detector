#include "pages/i2c_detector/i2c_scanner.hpp"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#ifdef __linux__
#include <dirent.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace {

#ifdef __linux__
std::vector<int> discover_bus_numbers()
{
    std::vector<int> buses;
    auto *directory = opendir("/dev");
    if (!directory) throw std::runtime_error(std::string("Cannot inspect /dev: ") + std::strerror(errno));

    while (const auto *entry = readdir(directory)) {
        constexpr const char *prefix = "i2c-";
        if (std::strncmp(entry->d_name, prefix, std::strlen(prefix)) != 0) continue;

        const char *number = entry->d_name + std::strlen(prefix);
        if (*number == '\0') continue;
        char *end         = nullptr;
        errno             = 0;
        const long parsed = std::strtol(number, &end, 10);
        if (errno == 0 && end && *end == '\0' && parsed >= 0 && parsed <= 1'000'000) {
            buses.push_back(static_cast<int>(parsed));
        }
    }
    closedir(directory);

    std::sort(buses.begin(), buses.end());
    buses.erase(std::unique(buses.begin(), buses.end()), buses.end());
    return buses;
}

bool probe_address(int file_descriptor, int address)
{
    if (ioctl(file_descriptor, I2C_SLAVE, address) < 0) return errno == EBUSY;

    union i2c_smbus_data data{};
    struct i2c_smbus_ioctl_data arguments{};
    arguments.read_write = I2C_SMBUS_WRITE;
    arguments.command    = 0;
    arguments.size       = I2C_SMBUS_QUICK;
    arguments.data       = &data;
    if (ioctl(file_descriptor, I2C_SMBUS, &arguments) == 0) return true;

    arguments.read_write = I2C_SMBUS_READ;
    arguments.size       = I2C_SMBUS_BYTE;
    return ioctl(file_descriptor, I2C_SMBUS, &arguments) == 0;
}

I2cScanReport scan_system_buses(const std::atomic_bool &cancelled)
{
    I2cScanReport report;
    for (const int bus_number : discover_bus_numbers()) {
        if (cancelled.load()) break;

        I2cBusScanResult result;
        result.bus_number      = bus_number;
        const std::string path = "/dev/i2c-" + std::to_string(bus_number);
        const int descriptor   = open(path.c_str(), O_RDWR | O_CLOEXEC);
        if (descriptor < 0) {
            result.error = std::strerror(errno);
            report.buses.push_back(std::move(result));
            continue;
        }

        for (int address = i2c_first_probe_address; address <= i2c_last_probe_address; ++address) {
            if (cancelled.load()) break;
            result.detected[static_cast<std::size_t>(address)] = probe_address(descriptor, address);
        }
        close(descriptor);
        report.buses.push_back(std::move(result));
    }
    return report;
}
#else
I2cScanReport scan_system_buses(const std::atomic_bool &)
{
    I2cScanReport report;
    report.error = "I2C scanning is only available on Linux devices";
    return report;
}
#endif

class SystemI2cScanner final : public I2cScanner {
public:
    ~SystemI2cScanner() override
    {
        cancelled_.store(true);
        if (worker_.joinable()) worker_.join();
    }

    bool start() override
    {
        if (running_.load()) return false;
        if (worker_.joinable()) worker_.join();

        {
            const std::lock_guard<std::mutex> guard(result_mutex_);
            pending_result_.reset();
        }
        cancelled_.store(false);
        running_.store(true);
        try {
            worker_ = std::thread([this] {
                I2cScanReport report;
                try {
                    report = scan_system_buses(cancelled_);
                } catch (const std::exception &error) {
                    report.error = error.what();
                } catch (...) {
                    report.error = "Unexpected error while scanning I2C buses";
                }

                if (!cancelled_.load()) {
                    const std::lock_guard<std::mutex> guard(result_mutex_);
                    pending_result_ = std::move(report);
                }
                running_.store(false);
            });
        } catch (...) {
            running_.store(false);
            throw;
        }
        return true;
    }

    [[nodiscard]] bool running() const noexcept override
    {
        return running_.load();
    }

    bool take_result(I2cScanReport &report) override
    {
        {
            const std::lock_guard<std::mutex> guard(result_mutex_);
            if (!pending_result_) return false;
            report = std::move(*pending_result_);
            pending_result_.reset();
        }
        if (worker_.joinable()) worker_.join();
        return true;
    }

private:
    std::atomic_bool running_{false};
    std::atomic_bool cancelled_{false};
    std::thread worker_;
    std::mutex result_mutex_;
    std::optional<I2cScanReport> pending_result_;
};

}  // namespace

std::unique_ptr<I2cScanner> make_system_i2c_scanner()
{
    return std::make_unique<SystemI2cScanner>();
}
