#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

if [[ "${EUID}" -ne 0 ]]; then
  echo "Run this installer with sudo: sudo bash scripts/setup-rpi4.sh" >&2
  exit 1
fi

if [[ ! -r /etc/os-release ]]; then
  echo "Cannot identify the operating system: /etc/os-release is missing or unreadable." >&2
  exit 1
fi

. /etc/os-release
if [[ "${ID:-}" != "raspbian" &&
      "${ID_LIKE:-}" != *"raspbian"* &&
      "${PRETTY_NAME:-}" != *"Raspberry Pi OS"* &&
      ! -e /etc/rpi-issue ]]; then
  echo "This installer requires Raspberry Pi OS; /etc/os-release does not identify it." >&2
  exit 1
fi

if [[ ! -r /proc/device-tree/model ]]; then
  echo "Cannot identify the hardware: /proc/device-tree/model is missing or unreadable." >&2
  exit 1
fi
model="$(tr -d '\000' < /proc/device-tree/model)"
if [[ "${model}" != *"Raspberry Pi 4"* ]]; then
  echo "This installer targets a Raspberry Pi 4; detected '${model:-unknown hardware}'." >&2
  exit 1
fi

apt-get update
apt-get install -y --no-install-recommends \
  build-essential \
  cmake \
  libpigpiod-if-dev \
  pigpio

systemctl enable --now pigpiod

build_dir="/opt/uv-roberta/build"
mkdir -p "${build_dir}"
cmake -S "${ROOT_DIR}" -B "${build_dir}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${build_dir}" --target roberta_rc_monitor --parallel
install -o root -g root -m 0755 \
  "${build_dir}/device/control/roberta_rc_monitor" \
  /usr/local/bin/roberta_rc_monitor

echo
echo "Installed the RC receiver monitor. It cannot command the motor controller."
echo
echo "Next:"
echo "  1. Confirm receiver signal voltage is safe for the Pi GPIO (3.3 V maximum)."
echo "  2. Connect SBUS signal to BCM GPIO 17 and share ground with the Pi."
echo "  3. Run: /usr/local/bin/roberta_rc_monitor"
echo "     Use --gpio BCM_PIN if your signal is connected to a different pin."
echo "  4. Move sticks and switches; observe SBUS channels 1-18 and link status."
