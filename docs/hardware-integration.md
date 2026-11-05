# Hardware integration and commissioning

## Known equipment

- Raspberry Pi 4
- FrSky Taranis X9 Lite transmitter and FrSky XM Plus receiver
- Roboteq MDC2250 controller connected over USB
- Two RioRand ZS-X11-H controllers for the alternate model
- Two powered-wheelchair drive motors
- USB GPS receivers and USB cameras

The previous-version source supplies initial receiver pin/channel and serial-framing clues, but actual wiring, channel mapping, motor polarity, calibration limits, and MDC2250 firmware configuration must still be verified on this robot. Keep motor output disabled until these are measured and documented. Do not assume the previous defaults match the current transmitter/controller setup.

## Phase 1: receiver link test

Phase 1 implements only a local SBUS receiver monitor. It captures inverted SBUS on Raspberry Pi BCM GPIO 17 by default and displays 16 analog channels, digital channels 17/18, frame freshness, frame-lost status, and receiver failsafe status. The monitor has no motor-controller interface. See [the Phase 1 setup guide](phase1-setup.md) for wiring and operation.

The GPIO pin default is based on previous-version source in `to-delete`; verify the actual signal pin and voltage before connecting it. Phase 1 does not test or command the Roboteq controller, drive motors, switches as features, network paths, GPS, or cameras.

The receiver pinout image identifies the XM+ signal, supply, and ground pads. The Raspberry Pi pinout confirms that the default BCM GPIO 17 is physical header pin 11. Refer to the [Phase 1 wiring instructions](phase1-setup.md); check signal voltage before connection because Pi GPIO is not 5 V tolerant.

## Phase 2: manual RC drive

The local `roberta_rc_drive` executable maps calibrated SBUS throttle and steering sticks to either the MDC2250's two motor channels or the pair of ZS-X11-H GPIO interfaces. Select only the controller connected to the current model with `--controller roboteq` or `--controller zs`; the other controller is not opened. The setup script installs the executable and an example config, but does not install or start a service. See [the Phase 2 drive guide](phase2-drive-setup.md) for channel mapping, calibration, manual startup, and commissioning.

Before motor power is enabled, verify the Roboteq serial settings and motor channel configuration, establish its host-loss watchdog behavior, and verify an independent physical emergency stop. For the ZS boards, verify the legacy GPIO mapping, external PWM input, direction logic, and active-low stop wiring against the actual board and robot. Keep the frame raised and command caps low for initial direction/stop tests. Treat example channel assignments and motor polarity as unverified until measured on this robot.

The ZS software defaults map left speed/direction/stop to BCM GPIO 12/24/22 and right speed/direction/stop to 13/23/27. See the [Phase 2 GPIO table and board wiring references](phase2-drive-setup.md#configure-and-calibrate). The diagrams show board connector functions and input requirements, but do not validate the Pi harness or board revision.

## ADC and power-module diagram

`images/ADC ADS1115 Diagram.png` is an unverified legacy sketch and must not be used as a wiring instruction. It labels pins 3 and 4 of a Pixhawk 5X/6X POWER connector as SCL and SDA; the POWER port uses analog current and voltage signals on those pins, not I²C. I²C must use the flight controller's I²C connector. The sketch also does not establish that a power-module output is within the ADS1115 analog input range. Do not wire this circuit as drawn; verify connector pinouts, signal scaling, supply voltage, and ground reference against the exact module and flight-controller documentation before designing or connecting an ADC.

The ADS1115/power-module images describe a separate telemetry circuit; it is not part of the current Raspberry Pi SBUS or motor-control wiring and is not read by the Phase 1–3 software.

## Phase 3: RC and Roboteq telemetry

`roberta_rc_monitor --telemetry --config PATH` displays all SBUS channels and read-only Roboteq measurements, including shared battery voltage and left/right speed, current, power, and temperature. The battery percentage is a configurable voltage estimate, not a fuel gauge. See [the Phase 3 telemetry guide](phase3-telemetry.md) for operation and interpretation.
