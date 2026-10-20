# UV Roberta v2 — RC receiver and manual drive

Phase 1 confirms the radio link:

`FrSky Taranis X9 Lite → XM+ SBUS → Raspberry Pi 4 console monitor`

The C++ monitor displays all 16 SBUS analog channels, digital channels 17/18, and reports stale frames, frame loss, and receiver failsafe. Phase 2 adds a separate manually started executable to control the Roboteq MDC2250 and wheels from the transmitter. Phase 3 adds an opt-in read-only combined RC and Roboteq telemetry screen. See [the Phase 1 setup guide](docs/phase1-setup.md) for receiver commissioning, [the Phase 2 drive guide](docs/phase2-drive-setup.md) for motor configuration and testing, and [the Phase 3 telemetry guide](docs/phase3-telemetry.md) for the combined monitor.

- `device/control/`: SBUS decoding, control policy, hardware-free tests, and the Pi monitor and drive executables.
- `scripts/setup-rpi4.sh`: installs Pi dependencies, both executables, and an example drive configuration.
- `docs/phase1-setup.md`: receiver wiring, install, and radio-link test procedure.
- `docs/phase2-drive-setup.md`: Roboteq configuration, stick calibration, and manual drive test procedure.
- `api/`, `web/`, and `device/agent/`: later-phase work, not used by the local RC drive executable.

Check receiver supply, common ground, and signal voltage before connecting to GPIO. Raspberry Pi GPIO is not 5 V tolerant; the SBUS signal must be verified at no more than 3.3 V. Before any motor test, follow the Phase 2 guide's independent emergency-stop and controller-watchdog checks.

## Build and test

Build the hardware-independent tests on a developer machine with CMake 3.20+ and a C++20 compiler:

```bash
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

On Raspberry Pi OS, follow [the Phase 1 setup guide](docs/phase1-setup.md). The monitor reads the SBUS signal on BCM GPIO 17 by default; select another pin with `--gpio`. The setup script also builds and installs the manual drive executable, but does not start it or install a service.
