# Phase 2: manual RC wheel control

Phase 2 adds direct transmitter control of the two Roboteq MDC2250 motor channels:

`Taranis X9 Lite → XM+ SBUS → Raspberry Pi → USB serial → Roboteq MDC2250 → drive motors`

The operator starts `roberta_rc_drive` in a terminal. There is no installed, enabled, or required drive service. The existing `roberta_rc_monitor` remains available for checking all SBUS channels without opening the motor controller.

## Safety before connecting drive power

- Keep the frame securely raised with both drive wheels clear of the ground for initial tests. Keep people, cables, and loose items clear of the wheels.
- Have a verified, independent physical motor-power emergency stop immediately accessible. Do not rely on Ctrl-C, the radio failsafe, or this program as the only emergency stop.
- Verify the Roboteq's serial protocol, baud rate, channel operating modes, motor wiring, and motor/controller configuration against the MDC2250 documentation and this robot's wiring. In particular, verify the controller's command watchdog/failsafe behavior for a disconnected or crashed host before powering the motors.
- Verify wheel direction with the drive wheels raised and the motor command limit kept low. Stop and correct the channel inversion/wiring if a wheel turns the wrong way. Do not test direction with the wheels on the ground.
- Keep transmitter failsafe configured to stop the receiver outputs. The runtime rejects SBUS failsafe and lost-frame data, stops both motor channels when the link goes stale, and disarms after link loss. A restored link does not resume motion: both sticks must be neutral for the configured arm hold time.

The program can command motor torque. A raised frame reduces the risk of unintended vehicle motion but does not eliminate pinch, entanglement, or electrical hazards.

## Install/build on Raspberry Pi OS

Follow the hardware checks and run the installer described in [the Phase 1 setup guide](phase1-setup.md). The installer builds and installs both `/usr/local/bin/roberta_rc_monitor` and `/usr/local/bin/roberta_rc_drive`, and places an example configuration at `/etc/uv-roberta/rc-drive.conf` only if one does not already exist. It does not create, enable, or start a service.

If the SBUS monitor is not already commissioned, first run:

```bash
/usr/local/bin/roberta_rc_monitor
```

Use the monitor to verify live receiver data and identify which SBUS channels correspond to the throttle and steering sticks. Exit with Ctrl-C.

## Configure and calibrate

Review the config:

```bash
sudoedit /etc/uv-roberta/rc-drive.conf
```

Set `receiver_gpio` to the BCM GPIO connected to SBUS. Set `throttle_channel` and `steering_channel` to the observed channel numbers (1–16); the example values are only placeholders. The Roboteq device defaults to `/dev/serial/by-id/usb-Roboteq_Motor_Controller_MDC2XXX-if00`; change `roboteq_device` in the config if this controller appears at another path. An empty `roboteq_device` setting also uses this default. Set `roboteq_baud` to the controller's configured baud rate. The configured left/right Roboteq channels must be distinct channels 1 and 2.

Keep `max_motor_command` at the conservative example value of 200 (20% of the controller's command range) or lower for commissioning. It is a software command cap, not a guarantee of a particular wheel speed or torque. Do not raise it until low-limit operation, motor directions, stop behavior, and the physical emergency stop have been verified.

Calibrate the sticks using the observed channels. The calibration process does not open or command the motor controller:

```bash
sudo /usr/local/bin/roberta_rc_drive \
  --config /etc/uv-roberta/rc-drive.conf \
  --calibrate
```

Follow the prompts to hold throttle at minimum, neutral, and maximum, then repeat for steering. The tool saves the captured endpoints and center to the configuration. Re-run calibration if transmitter endpoints or channel assignments change. If an axis acts in the wrong direction during the raised-wheel test, set that axis's `*_inverted=true`, save the config, and repeat the test.

Before opening the motor controller, verify the receiver mapping and calibration again in monitor mode:

```bash
/usr/local/bin/roberta_rc_drive \
  --config /etc/uv-roberta/rc-drive.conf \
  --monitor
```

This mode does not open the Roboteq or issue motor commands.

## Start and stop manual drive

With the frame still securely raised, people clear, the physical stop ready, and both sticks neutral, start the executable in a terminal:

```bash
/usr/local/bin/roberta_rc_drive \
  --config /etc/uv-roberta/rc-drive.conf \
  --arm
```

The program opens the Roboteq, sends zero commands, and waits for valid SBUS frames with both sticks neutral continuously for `arm_neutral_ms` (2 seconds by default). Once armed, throttle controls forward/reverse and steering mixes the left and right wheel commands. `deadband` suppresses small stick movement around center; per-axis inversion and per-motor inversion are independently configurable.

Move one stick at a time and confirm each wheel's direction at the low command limit. Then test steering, neutral stop, and transmitter-off/failsafe behavior. If the SBUS link is lost or marked failsafe, both channels are commanded to zero and the drive disarms. After restoring the link, return both sticks to neutral and wait for the full arm hold again before any motion is accepted.

Press Ctrl-C to exit; the program sends zero commands to both channels on normal shutdown. If it reports a serial or controller error, use the physical emergency stop and investigate before restarting. The runtime has no dashboard/network control path.

## Command reference

```text
roberta_rc_drive --config PATH --monitor
roberta_rc_drive --config PATH --calibrate
roberta_rc_drive --config PATH --arm
```

Exactly one mode is required. `--monitor` and `--calibrate` do not open the Roboteq. `--arm` requires a configured `roboteq_device`. No systemd unit is installed or started by the setup script; start the executable manually only when ready to test.
