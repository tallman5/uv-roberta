# Phase 3: RC and Roboteq telemetry

Phase 3 extends the console monitor to show the complete SBUS receiver state and Roboteq telemetry on one screen. It is an opt-in telemetry mode: it does not issue motor commands. Roboteq telemetry is polled by the controller's serial broadcast interval; receiver frames are optional.

## Run the combined monitor

Use the reviewed Phase 2 configuration and start telemetry mode:

```bash
/usr/local/bin/roberta_rc_monitor --telemetry \
  --config /etc/uv-roberta/rc-drive.conf
```

The monitor displays all 16 raw SBUS channels, digital channels 17/18, receiver freshness/failsafe state, and Roboteq readings for the configured left and right motor channels. The Roboteq section includes shared battery voltage, controller-reported speed, motor and battery current, applied power, motor/controller temperatures, and an estimated overall battery percentage. You can run it without a transmitter or `pigpiod`; the RC side then shows as unavailable while serial telemetry continues. Press Ctrl-C to exit.

The monitor uses `receiver_gpio`, `roboteq_device`, `roboteq_baud`, `left_motor_channel`, and `right_motor_channel` from the configuration. An explicit `--gpio BCM_PIN` overrides the configured receiver GPIO.

On serial startup, the monitor disables command echo, enables the `?A`, `?BA`, `?M`, `?T`, `?V`, and `?S` telemetry queries, and sets the broadcast interval to 100 ms, following the previous Roboteq integration's `# 100` polling behavior. It reads the continuing response stream and displays the latest values. The monitor does not send motor commands, disable the controller watchdog, or change motor mix mode. Stop any other program using the Roboteq serial device before starting telemetry mode.

Speed is the controller-reported RPM and depends on the controller's encoder feedback configuration; it may not be meaningful if speed feedback is not configured. If the screen reports `NO DATA`, verify `roboteq_device` and `roboteq_baud`, ensure the Roboteq is powered and no other process has the serial port open, and confirm the controller accepts the query commands and periodic broadcast configuration.

## Battery percentage

`battery_nominal_volts` defaults to 24 V, matching the existing 24 V wheelchair battery setup. The displayed percentage is a voltage-based estimate: it maps nominal voltage divided by 1.125 to 0% and nominal voltage multiplied by 1.125 to 100%, clamping values outside that range. It is not a fuel gauge and will vary with battery chemistry, temperature, and load. Set the nominal voltage to match the installed battery system; the estimate should not be used as a safety cutoff.

The MDC2250 exposes a shared battery-bus voltage, not an independent battery supply for each motor. The screen shows this same shared voltage alongside both left and right motor readings. It uses the second `?V` response value for the bus-voltage estimate, matching the previous Roboteq integration; verify the response against the installed MDC2250 firmware during commissioning.

## Preserve RC-only monitoring

The existing receiver-only monitor remains unchanged and does not open the Roboteq:

```bash
/usr/local/bin/roberta_rc_monitor
```

Use the combined telemetry screen only after checking the serial device and baud rate in the configuration. For motor control and commissioning safety, follow [the Phase 2 drive guide](phase2-drive-setup.md); telemetry mode itself never arms or drives the motors.
