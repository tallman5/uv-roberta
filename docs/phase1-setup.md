# Phase 1: RC transmitter and receiver test

Phase 1 has one purpose: confirm that the FrSky Taranis X9 Lite transmitter and XM+ receiver produce the expected SBUS channel data on the Raspberry Pi 4.

`Taranis X9 Lite → XM+ SBUS → Raspberry Pi console monitor`

The monitor displays all 16 analog channels, the SBUS digital channels 17 and 18, and whether frames are current, stale, lost, or marked failsafe. This monitor mode does not open or command the motor controller. For wheel control, first complete the monitor checks and then follow [the Phase 2 drive guide](phase2-drive-setup.md).

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

The installer checks for Raspberry Pi OS and Pi 4 hardware, updates installed OS packages, installs Git and any missing build/pigpio dependencies, ensures `pigpiod` is enabled and running, builds the monitor and manual drive executable, and installs both under `/usr/local/bin`. It creates `/etc/uv-roberta/rc-drive.conf` from the example only if no config exists; this file must be reviewed and configured before drive mode can open the Roboteq. It is safe to rerun: installed dependencies and a running/enabled daemon are detected, existing configuration is kept, and both executables are rebuilt. It does not install, configure, enable, or start a motor-control service. If a system update requires rebooting, the installer reports that at the end.

If you previously installed an older drive service, stop and disable it before testing in monitor-only mode:

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

The program connects to the local `pigpiod` daemon and redraws a fixed screen with one channel per line, updating values in place. It does not need `sudo` when pigpiod is running with its standard local socket access. If it cannot connect, check:

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

The monitor prints raw SBUS values; it does not assign semantic names to channels or calibrate endpoints. Record the channel numbers and observed ranges for the throttle and steering sticks. This information is needed to configure and calibrate drive mode in [Phase 2](phase2-drive-setup.md).

## If no valid frames appear

1. Recheck receiver power, binding, signal pin, common ground, and signal voltage.
2. Confirm the command uses the BCM GPIO number, not the physical header pin number.
3. Confirm `pigpiod` is active and the receiver output is SBUS rather than a different protocol.
4. Check that the SBUS signal is electrically compatible with the Pi GPIO. Use an appropriate inverter/level shifter if required by the receiver's output; do not experiment with a potentially 5 V signal connected directly.

This Phase 1 procedure does not configure or command the Roboteq or motors. Dashboard, API, GPS, and camera setup are also outside this receiver test. See [Phase 2](phase2-drive-setup.md) before attempting any wheel control.
