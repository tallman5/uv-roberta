#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build/pi-runtimes"

echo "Updating the repository..."
git -C "${ROOT_DIR}" pull --ff-only

echo "Configuring the Raspberry Pi runtimes..."
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DROBERTA_BUILD_PI_TARGETS=ON \
  -DROBERTA_BUILD_DRIVE_RUNTIME=ON

echo "Building the monitor and manual drive runtimes..."
cmake --build "${BUILD_DIR}" \
  --target roberta_rc_monitor roberta_rc_drive \
  --parallel

echo
echo "Built runtimes:"
echo "  ${BUILD_DIR}/device/control/roberta_rc_monitor"
echo "  ${BUILD_DIR}/device/control/roberta_rc_drive"
