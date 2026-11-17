#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build/pi-runtimes"

run_privileged() {
  if [[ "${EUID}" -eq 0 ]]; then
    "$@"
  else
    sudo "$@"
  fi
}

echo "Fetching repository updates..."
git -C "${ROOT_DIR}" fetch

echo "Discarding tracked local changes and syncing to the upstream branch..."
git -C "${ROOT_DIR}" reset --hard '@{upstream}'

echo "Configuring the Raspberry Pi runtimes..."
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DROBERTA_BUILD_PI_TARGETS=ON \
  -DROBERTA_BUILD_DRIVE_RUNTIME=ON

echo "Building the monitor and manual drive runtimes..."
cmake --build "${BUILD_DIR}" \
  --target roberta_rc_monitor roberta_rc_drive \
  --parallel

echo "Installing the Raspberry Pi runtimes..."
run_privileged install -o root -g root -m 0755 \
  "${BUILD_DIR}/device/control/roberta_rc_monitor" \
  /usr/local/bin/roberta_rc_monitor
run_privileged install -o root -g root -m 0755 \
  "${BUILD_DIR}/device/control/roberta_rc_drive" \
  /usr/local/bin/roberta_rc_drive
run_privileged chmod +x \
  /usr/local/bin/roberta_rc_monitor \
  /usr/local/bin/roberta_rc_drive

config_dir="/etc/uv-roberta"
config_file="${config_dir}/rc-drive.conf"
run_privileged install -d -o root -g root -m 0755 "${config_dir}"
if [[ ! -e "${config_file}" ]]; then
  run_privileged install -o root -g root -m 0644 \
    "${ROOT_DIR}/device/control/config/rc-drive.conf.example" \
    "${config_file}"
  echo "Installed an example drive config at ${config_file}; review and calibrate it before arming."
else
  echo "Keeping existing drive config at ${config_file}."
fi

echo "Ensuring pigpiod is enabled and running..."
run_privileged systemctl enable --now pigpiod.service

echo
echo "Installed runtimes:"
echo "  /usr/local/bin/roberta_rc_monitor"
echo "  /usr/local/bin/roberta_rc_drive"
echo "  ${config_file}"
echo
echo "To test the left ZS channel (uses built-in pin/PWM defaults):"
echo "  roberta_rc_drive --controller zs --test-controller --test-channel left"
