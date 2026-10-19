# UV Roberta v2 — Phase 1 MVP

The current MVP is **local RC-only driving**:

`FrSky Taranis X9 Lite → XM+ SBUS → Raspberry Pi 4 → Roboteq MDC2250 → drive motors`

Phase 1 does not use the API, dashboard, GPS, cameras, device agent, remote driving, or transmitter switches. Those are later-phase items.

- `device/control/`: C++20 SBUS capture/decoding, stick calibration, fail-safe control loop, and Roboteq USB serial output.
- `scripts/setup-rpi4.sh`: installs dependencies, builds and installs the Pi process, creates the config and manual-start systemd service.
- `docs/phase1-setup.md`: wiring, configuration, calibration, commissioning, and operation procedure.
- `api/`, `web/`, `device/agent/`, `shared/`: future-phase scaffolding; they are not required to run Phase 1.

## Safety

The motor service is not enabled at boot and will not restart automatically after a failure. Starting it requires an explicit `systemctl start`; both sticks must then remain neutral for two seconds before control arms. The shipped motor command limit is 200/1000 and must be treated as a commissioning limit, not a safe vehicle speed.

Receiver failsafe, invalid SBUS data, missing/stale frames, or a Roboteq serial write error stops drive commands locally. Use and independently verify the physical emergency stop and Roboteq command watchdog. Software stop behavior is not a substitute for either.

Do not arm until you have confirmed the receiver signal is safe for Pi GPIO (3.3 V maximum), common ground, channel mapping, stick calibration, motor polarity, MDC2250 serial framing, separate-motor mode, open-loop behavior, and controller watchdog configuration. The Pi 4 BCM GPIO 17 input and previous project channel assignments are starting points only.

## Build and test

Build the hardware-independent tests on a developer machine with CMake 3.20+ and a C++20 compiler:

```bash
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

On Raspberry Pi OS, follow [the Phase 1 setup guide](docs/phase1-setup.md). The native hardware target links to pigpio and is built by the setup script.
