# Hardware integration and commissioning

## Known equipment

- Raspberry Pi 4
- FrSky Taranis X9 Lite transmitter and FrSky XM Plus receiver
- Roboteq MDC2250 controller connected over USB
- Two powered-wheelchair drive motors
- USB GPS receivers and USB cameras

The previous-version source supplies initial receiver pin/channel and serial-framing clues, but actual wiring, channel mapping, motor polarity, calibration limits, and MDC2250 firmware configuration must still be verified on this robot. Keep motor output disabled until these are measured and documented. Do not assume the previous defaults match the current transmitter/controller setup.

## Phase 1 interfaces

The Phase 1 implementation provides:

- A pigpio GPIO-edge decoder for inverted XM+ SBUS on BCM GPIO 17.
- Configurable one-based throttle/steering channel mapping and an interactive stick-calibration mode.
- Local stale-frame/failsafe detection and differential mixing.
- Separate-channel open-loop Roboteq serial commands over USB.
- A manually started service with a neutral-stick arming interlock.

The receiver pin and channel defaults were recovered from previous-version source in `to-delete`. Verify them with the transmitter connected. The supported startup and calibration commands, Roboteq setup checks, and bench-test procedure are in [the Phase 1 setup guide](phase1-setup.md).

## Bench and hardware-in-the-loop checklist

1. Verify the physical stop and receiver/Pi signal voltage with the motors disconnected or wheels raised.
2. Use receiver monitor mode to identify the throttle and steering channels; calibrate neutral and endpoints.
3. Verify Roboteq USB serial settings, separate mode, `!G` open-loop behavior, controller watchdog, current limits, direction, and stop behavior with wheels raised.
4. Confirm forward/reverse/steer response at the initial low command limit and correct any polarity before putting the robot on the ground.
5. Remove receiver frames, switch off the transmitter, unplug USB, stop the process, and test the controller watchdog. Each case must stop both motor outputs.
6. Record hardware revision, wiring, channel ranges, serial settings, controller configuration, measured stop latency, and test results before operation.

Phase 1 does not use dashboard/API or network command paths. GPS and camera services remain later-phase work.
