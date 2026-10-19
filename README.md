# UV Roberta v2 — Phase 1 RC receiver test

Phase 1 is limited to confirming the radio link:

`FrSky Taranis X9 Lite → XM+ SBUS → Raspberry Pi 4 console monitor`

The C++ console app displays all 16 SBUS analog channels, digital channels 17/18, and reports stale frames, frame loss, and receiver failsafe. It cannot open or command the motor controller. No dashboard, API, GPS, cameras, device agent, or motor control is in Phase 1.

- `device/control/`: SBUS decoding, hardware-free decoder tests, and the Pi receiver monitor.
- `scripts/setup-rpi4.sh`: installs Pi dependencies and the monitor executable.
- `docs/phase1-setup.md`: receiver wiring, install, and radio-link test procedure.
- `api/`, `web/`, `device/agent/`, and the motor-control code: later-phase work, not used by Phase 1.

Check receiver supply, common ground, and signal voltage before connecting to GPIO. Raspberry Pi GPIO is not 5 V tolerant; the SBUS signal must be verified at no more than 3.3 V.

## Build and test

Build the hardware-independent tests on a developer machine with CMake 3.20+ and a C++20 compiler:

```bash
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

On Raspberry Pi OS, follow [the Phase 1 setup guide](docs/phase1-setup.md). The monitor reads the SBUS signal on BCM GPIO 17 by default; select another pin with `--gpio`.
