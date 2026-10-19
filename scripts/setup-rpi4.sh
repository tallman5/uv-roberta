#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
CONFIG_DIR="/etc/uv-roberta"
CONFIG_FILE="${CONFIG_DIR}/rc-drive.conf"
SERVICE_FILE="/etc/systemd/system/roberta-rc-drive.service"

if [[ "${EUID}" -ne 0 ]]; then
  echo "Run this installer with sudo: sudo bash scripts/setup-rpi4.sh" >&2
  exit 1
fi

if [[ ! -f /etc/os-release ]] || ! grep -qiE 'raspbian|raspberry' /etc/os-release; then
  echo "This installer targets Raspberry Pi OS on the Raspberry Pi 4." >&2
  # exit 1
fi

apt-get update
apt-get install -y --no-install-recommends \
  build-essential \
  cmake \
  libpigpiod-if-dev \
  pigpio

if ! id roberta >/dev/null 2>&1; then
  useradd --system --no-create-home --shell /usr/sbin/nologin roberta
fi
usermod -aG dialout roberta

systemctl enable --now pigpiod

mkdir -p "${CONFIG_DIR}"
chown root:roberta "${CONFIG_DIR}"
chmod 0750 "${CONFIG_DIR}"

if [[ ! -f "${CONFIG_FILE}" ]]; then
  shopt -s nullglob
  candidates=(/dev/serial/by-id/* /dev/ttyACM*)
  shopt -u nullglob
  detected_device=""
  if [[ "${#candidates[@]}" -eq 1 ]]; then
    detected_device="${candidates[0]}"
  fi

  read -r -p "MDC2250 USB serial path [${detected_device:-/dev/serial/by-id/...}]: " roboteq_device
  roboteq_device="${roboteq_device:-$detected_device}"
  read -r -p "Forward/reverse throttle SBUS channel [2]: " throttle_channel
  throttle_channel="${throttle_channel:-2}"
  read -r -p "Steering SBUS channel [1]: " steering_channel
  steering_channel="${steering_channel:-1}"
  read -r -p "MDC2250 serial baud rate (previous version used 9600) [9600]: " roboteq_baud
  roboteq_baud="${roboteq_baud:-9600}"

  temporary_config="$(mktemp)"
  trap 'rm -f "${temporary_config:-}"' EXIT
  cat > "${temporary_config}" <<EOF
# Calibrate with roberta_rc_drive --calibrate before arming.
receiver_gpio=17
throttle_channel=${throttle_channel}
throttle_min=172
throttle_center=992
throttle_max=1811
throttle_inverted=false
steering_channel=${steering_channel}
steering_min=172
steering_center=992
steering_max=1811
steering_inverted=false
deadband=0.04
receiver_timeout_ms=100
roboteq_device=${roboteq_device}
roboteq_baud=${roboteq_baud}
max_motor_command=200
left_motor_channel=2
right_motor_channel=1
left_motor_inverted=false
right_motor_inverted=false
arm_neutral_ms=2000
EOF
  install -o root -g roberta -m 0640 "${temporary_config}" "${CONFIG_FILE}"
  rm -f "${temporary_config}"
  trap - EXIT
fi
chown root:roberta "${CONFIG_FILE}"
chmod 0640 "${CONFIG_FILE}"

build_dir="/opt/uv-roberta/build"
mkdir -p "${build_dir}"
cmake -S "${ROOT_DIR}" -B "${build_dir}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${build_dir}" --target roberta_rc_drive --parallel
install -o root -g roberta -m 0755 \
  "${build_dir}/device/control/roberta_rc_drive" \
  /usr/local/bin/roberta_rc_drive

install -o root -g root -m 0644 \
  "${ROOT_DIR}/device/control/systemd/roberta-rc-drive.service" \
  "${SERVICE_FILE}"

systemctl daemon-reload
echo
echo "Installed roberta_rc_drive and the manually-started systemd service."
echo "The motor service has NOT been enabled at boot or started."
echo
echo "Next:"
echo "  1. Check wiring and motor-controller configuration in docs/phase1-setup.md."
echo "  2. Monitor channels: sudo -u roberta /usr/local/bin/roberta_rc_drive --config ${CONFIG_FILE} --monitor"
echo "  3. Calibrate sticks: sudo /usr/local/bin/roberta_rc_drive --config ${CONFIG_FILE} --calibrate"
echo "  4. Bench test with wheels raised, an accessible physical stop, and the configured low speed limit."
echo "  5. Start driving only after commissioning: sudo systemctl start roberta-rc-drive"
