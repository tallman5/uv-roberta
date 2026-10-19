# Hardware integration and commissioning

## Known equipment

- Raspberry Pi 4
- FrSky Taranis X9 Lite transmitter and FrSky XM Plus receiver
- Roboteq MDC2250 controller connected over USB
- Two powered-wheelchair drive motors
- USB GPS receivers and USB cameras

The previous-version source supplies initial receiver pin/channel and serial-framing clues, but actual wiring, channel mapping, motor polarity, calibration limits, and MDC2250 firmware configuration must still be verified on this robot. Keep motor output disabled until these are measured and documented. Do not assume the previous defaults match the current transmitter/controller setup.

## Phase 1: receiver link test

Phase 1 implements only a local SBUS receiver monitor. It captures inverted SBUS on Raspberry Pi BCM GPIO 17 by default and displays 16 analog channels, digital channels 17/18, frame freshness, frame-lost status, and receiver failsafe status. The monitor has no motor-controller interface. See [the Phase 1 setup guide](phase1-setup.md) for wiring and operation.

The GPIO pin default is based on previous-version source in `to-delete`; verify the actual signal pin and voltage before connecting it. Phase 1 does not test or command the Roboteq controller, drive motors, switches as features, network paths, GPS, or cameras.

## Phase 2: manual Roboteq drive

The local `roberta_rc_drive` executable maps calibrated SBUS throttle and steering sticks to the MDC2250's two motor channels. The setup script installs the executable and an example config, but does not install or start a service. See [the Phase 2 drive guide](phase2-drive-setup.md) for channel mapping, calibration, manual startup, and commissioning.

Before motor power is enabled, verify the controller's serial settings and motor channel configuration, establish its host-loss watchdog behavior, and verify an independent physical emergency stop. Keep the frame raised and the command cap low for initial direction/stop tests. Treat example channel assignments and motor polarity as unverified until measured on this robot.
