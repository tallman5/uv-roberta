# Phase 3: RC and Roboteq telemetry

Phase 3 extends the console monitor to show the complete SBUS receiver state and Roboteq telemetry on one screen. It is an opt-in, read-only mode: it sends only Roboteq query commands and does not issue motor commands or change controller settings.

## Run the combined monitor

Use the reviewed Phase 2 configuration and start telemetry mode:

```bash
/usr/local/bin/roberta_rc_monitor --telemetry \
  --config /etc/uv-roberta/rc-drive.conf
```

The monitor displays all 16 raw SBUS channels, digital channels 17/18, receiver freshness/failsafe state, and Roboteq readings for the configured left and right motor channels. The Roboteq section includes shared battery voltage, controller-reported speed, motor and battery current, applied power, motor/controller temperatures, and an estimated overall battery percentage. Press Ctrl-C to exit.

The monitor uses `receiver_gpio`, `roboteq_device`, `roboteq_baud`, `left_motor_channel`, and `right_motor_channel` from the configuration. An explicit `--gpio BCM_PIN` overrides the configured receiver GPIO.

The Roboteq telemetry uses its `?V`, `?S`, `?A`, `?BA`, `?M`, and `?T` query responses. Speed is the controller-reported RPM and depends on the controller's encoder feedback configuration; it may not be meaningful if speed feedback is not configured.

## Battery percentage

`battery_nominal_volts` defaults to 24 V, matching the existing 24 V wheelchair battery setup. The displayed percentage is a voltage-based estimate: it maps nominal voltage divided by 1.125 to 0% and nominal voltage multiplied by 1.125 to 100%, clamping values outside that range. It is not a fuel gauge and will vary with battery chemistry, temperature, and load. Set the nominal voltage to match the installed battery system; the estimate should not be used as a safety cutoff.

The MDC2250 exposes a shared battery-bus voltage, not an independent battery supply for each motor. The screen shows this same shared voltage alongside both left and right motor readings. It uses the second `?V` response value for the bus-voltage estimate, matching the previous Roboteq integration; verify the response against the installed MDC2250 firmware during commissioning.

## Preserve RC-only monitoring

The existing receiver-only monitor remains unchanged and does not open the Roboteq:

```bash
/usr/local/bin/roberta_rc_monitor
```

Use the combined telemetry screen only after checking the serial device and baud rate in the configuration. For motor control and commissioning safety, follow [the Phase 2 drive guide](phase2-drive-setup.md); telemetry mode itself never arms or drives the motors.
