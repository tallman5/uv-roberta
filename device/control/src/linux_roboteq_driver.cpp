#include "roberta/control/linux_roboteq_driver.hpp"

#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
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

int to_command(double value, int maximum, bool inverted) noexcept {
    const double adjusted = std::clamp(value, -1.0, 1.0) * (inverted ? -1.0 : 1.0);
    return static_cast<int>(adjusted * maximum);
}

}  // namespace

LinuxRoboteqDriver::LinuxRoboteqDriver(
    std::string device,
    int baud,
    int max_command,
    int left_channel,
    int right_channel,
    bool invert_left,
    bool invert_right)
    : max_command_(max_command),
      left_channel_(left_channel),
      right_channel_(right_channel),
      invert_left_(invert_left),
      invert_right_(invert_right) {
    if (device.empty() || max_command_ < 1 || max_command_ > 1000 ||
        (left_channel_ != 1 && left_channel_ != 2) ||
        (right_channel_ != 1 && right_channel_ != 2) ||
        left_channel_ == right_channel_) {
        throw std::invalid_argument("Invalid Roboteq device or maximum command.");
    }

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
    const auto speed = baud_constant(baud);
    if (cfsetispeed(&options, speed) != 0 ||
        cfsetospeed(&options, speed) != 0 ||
        tcsetattr(fd_, TCSANOW, &options) != 0) {
        close(fd_);
        fd_ = -1;
        throw std::runtime_error("Cannot configure the Roboteq serial port.");
    }
    tcflush(fd_, TCIOFLUSH);
}

LinuxRoboteqDriver::~LinuxRoboteqDriver() {
    stop();
    if (fd_ >= 0) {
        close(fd_);
    }
}

bool LinuxRoboteqDriver::healthy() const noexcept {
    return fd_ >= 0 && !faulted_;
}

bool LinuxRoboteqDriver::send_text(const std::string& text) noexcept {
    if (fd_ < 0 || text.empty()) {
        return false;
    }

    std::size_t sent = 0;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(50);
    while (sent < text.size()) {
        const auto result = write(fd_, text.data() + sent, text.size() - sent);
        if (result > 0) {
            sent += static_cast<std::size_t>(result);
            continue;
        }
        if (result < 0 && errno != EAGAIN && errno != EINTR) {
            return false;
        }

        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now()).count();
        if (remaining <= 0) {
            return false;
        }
        pollfd descriptor{fd_, POLLOUT, 0};
        if (poll(&descriptor, 1, static_cast<int>(remaining)) <= 0 ||
            (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
            return false;
        }
    }
    return true;
}

bool LinuxRoboteqDriver::send_command(int channel, int command) noexcept {
    if (channel < 1 || channel > 2 || command < -1000 || command > 1000) {
        return false;
    }
    return send_text(
        "!G " + std::to_string(channel) + " " + std::to_string(command) + "\r");
}

bool LinuxRoboteqDriver::drain_input() noexcept {
    if (fd_ < 0) {
        return false;
    }

    std::array<char, 256> buffer{};
    bool accepted = true;
    for (;;) {
        const auto count = read(fd_, buffer.data(), buffer.size());
        if (count > 0) {
            for (ssize_t index = 0; index < count; ++index) {
                const char value = buffer[static_cast<std::size_t>(index)];
                if (value == '\r' || value == '\n') {
                    if (input_line_ == "-") {
                        accepted = false;
                    }
                    input_line_.clear();
                } else if (input_line_.size() < 256) {
                    input_line_.push_back(value);
                } else {
                    return false;
                }
            }
            continue;
        }
        if (count == 0 || errno == EAGAIN || errno == EWOULDBLOCK) {
            break;
        }
        if (errno != EINTR) {
            return false;
        }
    }
    return accepted;
}

bool LinuxRoboteqDriver::apply(const MotorOutput& output) noexcept {
    if (!healthy() || !drain_input()) {
        faulted_ = true;
        stop();
        return false;
    }
    if (!send_command(left_channel_, to_command(output.left, max_command_, invert_left_))) {
        faulted_ = true;
        stop();
        return false;
    }
    if (!send_command(right_channel_, to_command(output.right, max_command_, invert_right_))) {
        faulted_ = true;
        stop();
        return false;
    }
    return true;
}

void LinuxRoboteqDriver::stop() noexcept {
    if (fd_ < 0) {
        return;
    }
    const bool responses_accepted = drain_input();
    const bool left_stopped = send_command(left_channel_, 0);
    const bool right_stopped = send_command(right_channel_, 0);
    faulted_ = faulted_ || !responses_accepted || !left_stopped || !right_stopped;
}

}  // namespace roberta::control
