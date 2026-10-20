#include "roberta/control/linux_roboteq_telemetry.hpp"

#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <chrono>
#include <initializer_list>
#include <stdexcept>
#include <string>

namespace roberta::control {
namespace {

speed_t baud_constant(int baud) {
    switch (baud) {
    case 9600: return B9600;
    case 19200: return B19200;
    case 38400: return B38400;
    case 57600: return B57600;
    case 115200: return B115200;
    default: throw std::invalid_argument("Unsupported Roboteq baud rate.");
    }
}

constexpr auto kWriteTimeout = std::chrono::milliseconds(250);
constexpr auto kRefreshWindow = std::chrono::milliseconds(200);

}  // namespace

LinuxRoboteqTelemetry::LinuxRoboteqTelemetry(std::string device, int baud) {
    if (device.empty()) {
        throw std::invalid_argument("Roboteq serial device must not be empty.");
    }
    const auto speed = baud_constant(baud);

    fd_ = open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (fd_ < 0) {
        throw std::runtime_error("Cannot open Roboteq serial device " + device + ".");
    }

    termios options{};
    if (tcgetattr(fd_, &options) != 0) {
        close(fd_);
        fd_ = -1;
        throw std::runtime_error("Cannot read Roboteq serial settings.");
    }

    cfmakeraw(&options);
    options.c_cflag &= ~(CSIZE | PARODD | CSTOPB | CRTSCTS);
    options.c_cflag |= CS7 | PARENB | CLOCAL | CREAD;
    options.c_iflag |= INPCK;
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 0;
    if (cfsetispeed(&options, speed) != 0 ||
        cfsetospeed(&options, speed) != 0 ||
        tcsetattr(fd_, TCSANOW, &options) != 0) {
        close(fd_);
        fd_ = -1;
        throw std::runtime_error("Cannot configure the Roboteq serial port.");
    }
    tcflush(fd_, TCIOFLUSH);
    try {
        for (const auto command : {
                 "^ECHOF 1", "?A", "?A", "?BA", "?M", "?T", "?V", "?S", "# 100"}) {
            if (!send_command(command)) {
                throw std::runtime_error("Timed out configuring Roboteq telemetry broadcast.");
            }
        }
    } catch (...) {
        close(fd_);
        fd_ = -1;
        throw;
    }
}

LinuxRoboteqTelemetry::~LinuxRoboteqTelemetry() {
    if (fd_ >= 0) {
        close(fd_);
    }
}

bool LinuxRoboteqTelemetry::send_command(std::string_view command_text) {
    std::string command(command_text);
    command.push_back('\r');
    std::size_t sent{};
    const auto deadline = std::chrono::steady_clock::now() + kWriteTimeout;
    while (sent < command.size()) {
        const auto result = write(fd_, command.data() + sent, command.size() - sent);
        if (result > 0) {
            sent += static_cast<std::size_t>(result);
            continue;
        }
        if (result < 0 && errno != EAGAIN && errno != EINTR) {
            throw std::runtime_error("Failed writing a Roboteq telemetry command.");
        }

        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now()).count();
        if (remaining <= 0) {
            return false;
        }
        pollfd descriptor{fd_, POLLOUT, 0};
        if (poll(&descriptor, 1, static_cast<int>(remaining)) < 0 && errno != EINTR) {
            throw std::runtime_error("Failed waiting to write a Roboteq telemetry command.");
        }
        if ((descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
            throw std::runtime_error("Roboteq serial connection failed while configuring telemetry.");
        }
    }
    return true;
}

std::optional<std::string> LinuxRoboteqTelemetry::read_line(
    std::chrono::steady_clock::time_point deadline) {
    std::array<char, 256> buffer{};
    for (;;) {
        const auto terminator = input_buffer_.find_first_of("\r\n");
        if (terminator != std::string::npos) {
            std::string line = input_buffer_.substr(0, terminator);
            input_buffer_.erase(0, terminator + 1);
            if (!line.empty()) {
                return line;
            }
            continue;
        }

        if (input_buffer_.size() >= 256) {
            throw std::runtime_error("Roboteq telemetry response exceeded the line limit.");
        }
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now()).count();
        if (remaining <= 0) {
            return std::nullopt;
        }

        pollfd descriptor{fd_, POLLIN, 0};
        const int result = poll(&descriptor, 1, static_cast<int>(remaining));
        if (result == 0) {
            return std::nullopt;
        }
        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw std::runtime_error("Failed waiting for Roboteq telemetry response.");
        }
        if ((descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
            throw std::runtime_error("Roboteq serial connection failed while reading telemetry.");
        }
        const auto count = read(fd_, buffer.data(), buffer.size());
        if (count > 0) {
            input_buffer_.append(buffer.data(), static_cast<std::size_t>(count));
        } else if (count < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            throw std::runtime_error("Failed reading Roboteq telemetry response.");
        }
    }
}

RoboteqTelemetrySnapshot LinuxRoboteqTelemetry::refresh() {
    const auto deadline = std::chrono::steady_clock::now() + kRefreshWindow;
    for (;;) {
        const auto line = read_line(deadline);
        if (!line) {
            break;
        }

        bool recognized = false;
        if (const auto values = parse_roboteq_values(*line, "V=", 0.1);
            values && values->size() >= 2) {
            latest_snapshot_.battery_volts = (*values)[1];
            recognized = true;
        } else if (const auto values = parse_roboteq_pair(*line, "S=")) {
            latest_snapshot_.speed_rpm = {(*values)[0], (*values)[1]};
            recognized = true;
        } else if (const auto values = parse_roboteq_pair(*line, "A=", 0.1)) {
            latest_snapshot_.motor_amps = {(*values)[0], (*values)[1]};
            recognized = true;
        } else if (const auto values = parse_roboteq_pair(*line, "BA=", 0.1)) {
            latest_snapshot_.battery_amps = {(*values)[0], (*values)[1]};
            recognized = true;
        } else if (const auto values = parse_roboteq_pair(*line, "M=", 0.1)) {
            latest_snapshot_.power_percent = {(*values)[0], (*values)[1]};
            recognized = true;
        } else if (const auto values = parse_roboteq_triple(*line, "T=")) {
            latest_snapshot_.controller_temperature = (*values)[0];
            latest_snapshot_.motor_temperature = {(*values)[1], (*values)[2]};
            recognized = true;
        }
        if (recognized) {
            ++latest_snapshot_.responses;
            latest_snapshot_.last_response_at = std::chrono::steady_clock::now();
        }
    }
    return latest_snapshot_;
}

}  // namespace roberta::control
