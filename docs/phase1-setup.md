# Phase 1: RC-only driving setup

Phase 1 is a local, manual-control MVP: FrSky Taranis X9 Lite → XM+ SBUS → Raspberry Pi 4 → Roboteq MDC2250 → two drive motors. It does not run the dashboard, API, device agent, GPS, cameras, slow-mode switch, or remote control.

The RC input is read on BCM GPIO 17 using pigpio edge timing. The previous code used this pin and decoded the XM+ inverted SBUS signal directly. The MDC2250 is driven over USB serial with separate `!G` commands for channels 1 and 2. This project does not write permanent Roboteq configuration.

## Before powering the motors

1. Secure the robot on stands with all drive wheels clear of the floor.
2. Have the physical emergency stop or motor power disconnect within immediate reach. The software stop is not an emergency stop.
3. Check the receiver's supply requirements and common ground. Confirm the receiver signal voltage is safe for a Pi GPIO input (3.3 V maximum); do not connect a 5 V signal to a GPIO.
4. Connect XM+ SBUS signal to Raspberry Pi BCM GPIO 17 (physical header pin 11), receiver ground to Pi ground, and provide the receiver's correctly rated supply.
5. Connect the MDC2250 to the Pi with USB. Leave the motors mechanically disconnected or raised until the receiver and motor directions have been checked.
6. The previous receiver code used SBUS channel 1 for steering and channel 2 for throttle. The configuration starts with those channels only as a convenient starting point; verify your Taranis model/output mapping in monitor mode. Channels 17/18 and transmitter switches are not used in this phase.

Do not proceed if the signal voltage, ground, motor polarity, physical stop, or controller configuration is unknown.

## Install and build on Raspberry Pi OS

Clone/copy this repository onto the Pi, then run:

```bash
cd uv-roberta-v2
sudo bash scripts/setup-rpi4.sh
```

The setup script:

- Installs the C++ build tools and pigpio client/daemon packages.
- Creates the unprivileged `roberta` service account and adds it to the serial-device group.
- Starts pigpiod, builds `roberta_rc_drive`, and installs a manually started systemd service.
- Prompts for the MDC2250 device path, throttle/steering SBUS channel numbers, and serial baud rate.
- Creates `/etc/uv-roberta/rc-drive.conf` only if one does not already exist; it preserves an existing configuration.

It does **not** start or enable the motor-control service. The initial motor command limit is 200 out of 1000. This is only a command limit, not a guarantee of a safe vehicle speed.

The setup script expects Raspberry Pi OS and internet access for apt. If the Pi was already running an old service, stop that service before continuing so only one process can command the MDC2250.

## Confirm receiver input and calibrate the sticks

First inspect the raw channels. This mode does not open or command the motor controller:

```bash
sudo -u roberta /usr/local/bin/roberta_rc_drive \
  --config /etc/uv-roberta/rc-drive.conf \
  --monitor
```

Turn on the transmitter and check that:

- Fresh `SBUS CH1=... CH2=...` readings are printed and change when you move the sticks.
- The throttle stick changes the configured throttle channel and the steering stick changes the configured steering channel.
- Stick centers and both endpoints are stable.
- Switching off the transmitter results in `No fresh valid receiver frame` rather than a latched command.

If your channel assignments differ, edit `throttle_channel` and `steering_channel` in `/etc/uv-roberta/rc-drive.conf` (one-based SBUS numbers, 1 through 16), then rerun monitor mode.

Calibrate actual center and endpoints; this writes only the RC configuration and does not open the motor controller:

```bash
sudo /usr/local/bin/roberta_rc_drive \
  --config /etc/uv-roberta/rc-drive.conf \
  --calibrate
```

Follow the prompts for throttle minimum, neutral, maximum, then steering minimum, neutral, maximum. Hold each position while the program samples the receiver. Calibration is rejected if ranges are too narrow or outside SBUS's expected raw channel range. The measured values are written to the config file. Use `throttle_inverted=true` or `steering_inverted=true` if a stick's direction is opposite to the intended forward/right direction.

## Configure and verify the MDC2250

Before arming, use Roboteq's configuration tool and the controller manual for the installed firmware to verify:

- USB serial is available at the configured baud rate. The installer defaults to 9600, matching the previous project's serial settings; change it if the controller is configured differently.
- Serial framing matches the configured 7 data bits, even parity, and 1 stop bit.
- Motor mixing is **separate** (Roboteq mix mode 0); the program sends `!G 1 <command>` and `!G 2 <command>`.
- Both channels are configured for the intended open-loop speed behavior.
- Both motors, polarity, controller current limits, battery limits, and controller stop behavior have been checked with wheels raised.
- The controller's command watchdog is enabled and tested to stop output after lost serial commands. Do not disable the watchdog.

The previous project's code disabled the Roboteq watchdog in one service. Do not carry that setting into this installation. This project deliberately does not send controller-configuration commands on startup.

Edit `/etc/uv-roberta/rc-drive.conf` if necessary:

```text
roboteq_device=/dev/serial/by-id/<the-MDC2250-USB-device>
roboteq_baud=9600
max_motor_command=200
left_motor_channel=2
right_motor_channel=1
left_motor_inverted=false
right_motor_inverted=false
```

The setup script prints discovered serial-device candidates and asks for the MDC2250 path. Prefer its stable `/dev/serial/by-id/...` name to `/dev/ttyACM0`. Previous-version telemetry treated Roboteq channel 1 as the right motor and channel 2 as the left; verify this against the actual wiring. The channel assignments are configurable. Set motor inversion only after individually verifying each wheel direction. Start at the shipped command limit; increase it only in small, controlled steps with the robot restrained.

## Bench test and start

1. Keep the wheels raised and the physical stop immediately reachable.
2. Check `sudo systemctl status pigpiod` and run receiver monitor mode again.
3. Start the drive service manually:

   ```bash
   sudo systemctl start roberta-rc-drive
   sudo journalctl -u roberta-rc-drive -f
   ```

4. The service sends zero commands and waits for both sticks to remain neutral for two seconds. Move one axis only and verify the corresponding wheel direction at the low configured command limit.
5. Test forward, reverse, left, and right. If direction is wrong, stop the service and correct the axis/motor inversion in config.
6. Test transmitter power-off/range loss. The local controller must send zero commands when receiver frames are stale, lost, invalid, or flagged failsafe.
7. Test `sudo systemctl stop roberta-rc-drive`; confirm the MDC2250 stops. Verify the controller watchdog separately by disconnecting the Pi USB cable while the wheels are raised.
8. Only after successful bench tests, place the robot on the ground in a clear area with a spotter and the physical stop ready.

Useful commands:

```bash
sudo systemctl stop roberta-rc-drive
sudo systemctl status roberta-rc-drive
sudo journalctl -u roberta-rc-drive -n 100 --no-pager
```

The service is intentionally not enabled at boot and does not restart automatically after a failure. Start it manually after checking the robot and transmitter. Do not use `systemctl enable` for Phase 1.

## What the program does when inputs fail

- Startup requires `--arm` (the service uses it) and waits for fresh, valid neutral stick frames.
- Stick calibration, SBUS framing/parity, selected channel ranges, SBUS frame-lost/failsafe flags, and receiver freshness are checked locally.
- Invalid, missing, or stale receiver input results in zero commands to both motor channels.
- A serial write failure stops the control process; systemd will not automatically restart it.
- Shutdown sends zero commands. The Roboteq watchdog and physical stop remain independent protections.

If receiver readings are absent, verify the receiver supply/ground, signal voltage, BCM-versus-physical pin numbering, transmitter binding, and the GPIO pin number. If the serial device cannot open, inspect `ls -l /dev/serial/by-id/` and confirm the `roberta` account has serial-device access. If the robot moves in the wrong direction, stop it before changing motor/axis polarity.
