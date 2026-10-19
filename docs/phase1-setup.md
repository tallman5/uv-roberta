# Phase 1: RC transmitter and receiver test

Phase 1 has one purpose: confirm that the FrSky Taranis X9 Lite transmitter and XM+ receiver produce the expected SBUS channel data on the Raspberry Pi 4.

`Taranis X9 Lite → XM+ SBUS → Raspberry Pi console monitor`

The monitor displays all 16 analog channels, the SBUS digital channels 17 and 18, and whether frames are current, stale, lost, or marked failsafe. It has no motor-controller interface and cannot drive the robot. Do not connect or power the drive motors for this test.

## Wiring and safety

1. Identify the XM+ receiver's SBUS output, ground, and required supply voltage using its documentation.
2. Make sure the Pi and receiver share ground.
3. Verify with a meter or level shifter documentation that the SBUS signal reaching the Pi is no more than 3.3 V. Raspberry Pi GPIO pins are not 5 V tolerant.
4. Connect SBUS signal to BCM GPIO 17 (physical header pin 11), or note the BCM pin you choose.
5. Do not connect the signal until the wiring and voltage are verified. Do not connect the receiver to a Pi GPIO if its output voltage is unknown or above 3.3 V.

The BCM GPIO 17 default is based on prior-version code. Use `--gpio BCM_PIN` if your receiver is wired to another GPIO.

## Install on Raspberry Pi OS

Copy or clone the repository on the Pi and run:

```bash
cd uv-roberta-v2
sudo bash scripts/setup-rpi4.sh
```

The installer checks for Raspberry Pi OS and Pi 4 hardware, installs the build and pigpio dependencies, starts `pigpiod`, builds the monitor, and installs it as `/usr/local/bin/roberta_rc_monitor`. It does not configure or start a motor-control service and does not create a drive configuration.

If you previously installed the drive service, stop and disable it before this RC-only test:

```bash
sudo systemctl disable --now roberta-rc-drive.service
```

## Run the monitor

With the transmitter powered on and bound to the receiver, run:

```bash
roberta_rc_monitor
```

If your SBUS signal uses another BCM GPIO:

```bash
roberta_rc_monitor --gpio 18
```

The program connects to the local `pigpiod` daemon and refreshes the console display. It does not need `sudo` when pigpiod is running with its standard local socket access. If it cannot connect, check:

```bash
sudo systemctl status pigpiod
```

Stop the monitor with Ctrl-C.

## Check transmitter and receiver behavior

- Move each stick individually and confirm that one or more channel values change in the expected direction.
- Operate each transmitter switch and identify which channel changes and the values at each switch position.
- Confirm channel values are stable while controls are held still and return to their expected neutral/center values.
- Turn off the transmitter briefly. The display should indicate stale frames or receiver failsafe rather than implying a live healthy signal.
- Turn the transmitter back on and confirm fresh channel values return.

The monitor prints raw SBUS values; it does not assign semantic names to channels or calibrate endpoints. Record the channel numbers and observed ranges for sticks and switches. This information will inform a later control phase.

## If no valid frames appear

1. Recheck receiver power, binding, signal pin, common ground, and signal voltage.
2. Confirm the command uses the BCM GPIO number, not the physical header pin number.
3. Confirm `pigpiod` is active and the receiver output is SBUS rather than a different protocol.
4. Check that the SBUS signal is electrically compatible with the Pi GPIO. Use an appropriate inverter/level shifter if required by the receiver's output; do not experiment with a potentially 5 V signal connected directly.

No Roboteq, motor, dashboard, API, GPS, or camera setup is part of Phase 1.
