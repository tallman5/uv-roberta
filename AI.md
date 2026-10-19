# AI Context for UV Roberta v2

## Purpose and scope

UV Roberta v2 is a Raspberry Pi 4–based, remotely observable and controllable wheeled robot. It retains the existing hardware class:

- Repurposed powered wheelchair base, 24v with two drive motors
- Raspberry Pi 4
- Motor controller, Roboteq MDC2250, connected to the RPi4 via USB
- USB GPS receivers
- USB Web cameras
- RC Transmitter, FrSky Taranis X9 Lite
- RC receiver, FrSky XM Plus Mini Receiver up to 16CH 1.6g Full Range, connected to the RPi4 via pins

### Current implementation phase

Phase 1 is limited to testing the RC transmitter/receiver link: Taranis X9 Lite → XM+ SBUS → Raspberry Pi 4 console monitor. The monitor displays SBUS channels 1-16 and digital channels 17/18, along with signal/failsafe status; it must not control or open the motor controller. Do not add motor driving, GPS, cameras, API/hub, dashboard, or remote control to the Phase 1 runtime. Keep the other architecture and contracts below as later-phase context; they are not required to test the Phase 1 RC link.

The software direction is to use C++ for low-level, timing-sensitive hardware control wherever it is practical, and Qwik with Bootstrap for the web dashboard.

## Design priorities

1. **Local safety first.** Motor stop, RC failsafe, and other essential safety behavior must continue to work if the network, dashboard, or API is unavailable.
2. **Keep hardware control on the Pi.** Hardware-facing code should run next to the devices and should not depend on cloud round trips for control-loop timing.
3. **Use C++ at the hardware boundary.** Prefer C++ for RC receiver decoding, motor command generation, Roboteq serial communication, and other timing-sensitive or Pi-specific work. Use another language where it materially improves maintainability or integration.
4. **Separate deployable components, not necessarily repositories.** The dashboard, API/hub, and Pi software may live in one repository while remaining separately buildable and deployable.
5. **Make interfaces explicit.** Define versioned, validated messages for telemetry and remote commands. Treat every incoming remote command as untrusted and subject to local safety limits.

## Proposed architecture

### Raspberry Pi device software

The Pi hosts the hardware agent and device services:

- A C++ console utility reads the RC receiver for bring-up and testing. Motor control is a later phase.
- GPS receivers are read locally; the software validates and timestamps fixes before publishing them.
- Camera streaming is handled by a camera/streaming service such as the existing `ustreamer` setup, rather than by the motor control loop.
- A device agent sends telemetry to the API/hub and receives authorized operator commands.

The control process must remain safe if the device agent or network disconnects. RC input and local failsafe logic take precedence over remote control. Remote control should have an explicit operating mode, bounded commands, stale-command timeout, and a reliable stop path.

The device agent can be a separate process from the control loop. Use a narrow local interface between them (for example, a local socket or another documented IPC mechanism) so that network and authentication code cannot stall hardware control.

### API and real-time hub

The API/hub is the authenticated coordination point between the dashboard and the Pi device agent. It is responsible for:

- Authentication and authorization of operators and devices
- Relaying validated control commands to the device agent
- Publishing live telemetry and connection status
- Providing ordinary HTTP endpoints for non-real-time operations as needed
- Enforcing command-level authorization, limits, and audit-friendly logging

Keep the hub independent of direct GPIO and serial device access when it is hosted away from the Pi. The Pi-side agent owns hardware access and applies final safety checks regardless of what the API authorizes.

### Web dashboard

Build the operator dashboard with Qwik and Bootstrap.

- Use Qwik for application structure, routing, and interactive UI.
- Use Bootstrap components and responsive layout conventions for visual consistency and mobile support.
- Keep connection state, telemetry presentation, and operator controls clearly separated.
- Provide a virtual thumbstick for dashboard driving, with bounded directional input and an explicit stop/neutral state.
- Show connection loss, stale telemetry, and command rejection explicitly.
- Never imply a command succeeded until the device acknowledges it.
- Keep camera streams as separately served media URLs; do not proxy high-bandwidth video through the hub unless there is a specific requirement.

## Hub placement: API folder versus static website

A possible layout is:

```text
web/       Qwik + Bootstrap site; builds to static assets
api/       HTTP API and real-time hub; independently deployable
device/    Raspberry Pi device agent
hardware/  C++ control and hardware-facing libraries, if shared independently
```

The important distinction is **source-code location versus runtime hosting**:

- A static site host serves generated files; it does not run a persistent SignalR/WebSocket hub merely because hub code is stored beside the site.
- The `api/` folder must be deployed to a runtime that supports the hub's connection model and required WebSocket behavior. It can be a separate app/service deployment from the static site without being a separate repository.
- If a serverless platform is selected, verify its WebSocket, connection duration, scaling, and SignalR support before choosing it. A managed real-time service can handle client connections, but the Pi still needs a persistent device-side connection or another reliable command/telemetry transport.

Recommended default: keep `web/` and `api/` in one repository, build and deploy them independently, and keep the Pi device software as its own deployable component. Avoid coupling motor control to static-site hosting.

## Data flow

1. The C++ control process reads RC input and applies local failsafe and motor limits.
2. The control process communicates with the Roboteq controller and reports local status.
3. GPS and controller telemetry are timestamped and sent by the Pi device agent to the API/hub.
4. The hub broadcasts current state to authenticated dashboard clients.
5. An authorized operator sends a bounded command through the hub.
6. The Pi device agent forwards the command to the local control process.
7. The control process validates the command, applies local safety rules, controls the Roboteq, and reports acknowledgement/state.

Camera streams follow a separate path from control and telemetry. They should not block, delay, or share a failure domain with the motor-control loop.

## Interface and safety requirements

- Keep the RC receiver as a local control and failsafe path; network access must not be required to stop the robot.
- Define a clear priority policy between RC and remote control. On ambiguous state, stale input, or loss of communications, stop or return control to the safe local mode.
- The RC transmitter has switches in addition to its normal drive sticks. Provide one switch for slow mode, which limits Roberta to 25% of full speed, and another switch to select whether the RC transmitter or the web dashboard has driving control. The dashboard driving control is a virtual thumbstick. Do not blend inputs: only the selected source may drive, and loss or staleness of the selected source must result in a safe stop.
- Apply the slow-mode limit to the final device-side motor command regardless of whether driving control comes from the RC transmitter or the dashboard; do not treat the switch as a substitute for existing command limits or failsafe behavior.
- Validate command range, operating mode, authorization, timestamp/expiry, and device identity at the relevant boundaries.
- Add a device-side command timeout so loss of hub connectivity cannot leave a movement command latched indefinitely.
- Use monotonic time for local timeout and control-loop calculations; include wall-clock timestamps only where useful for telemetry and diagnostics.
- Do not let telemetry serialization, logging, or network I/O block time-sensitive motor or receiver processing.
- Use bounded queues and explicit handling for dropped or stale telemetry/commands.
- Do not commit credentials, tokens, private keys, or production connection strings. Load secrets from the runtime environment or an approved secret store.
- Keep API and device credentials distinct, scoped, and revocable.

## Suggested repository organization

This is a target organization, not a request to move existing files immediately:

```text
device/
  agent/              Pi-side transport and orchestration
  control/            C++ control loop and safety logic
  drivers/            RC, Roboteq, GPS, and other hardware interfaces
api/
  ...                 Authenticated HTTP API and real-time hub
web/
  ...                 Qwik dashboard with Bootstrap
shared/
  contracts/          Versioned telemetry and command schemas
docs/
  ...                 Hardware setup, deployment, and operations
```

Keep hardware-specific code behind small interfaces so it can be tested with simulated inputs and fake devices. Avoid a large cross-language shared library unless the same implementation genuinely needs to be consumed by multiple components; a language-neutral message schema is usually a simpler boundary.

## Development and verification guidance

- Build C++ code on the Pi or a compatible Linux environment with the required GPIO and serial dependencies. Keep hardware-free unit tests runnable on a developer machine where practical.
- Build the API and dashboard independently; deployment of one should not require rebuilding or restarting unrelated components.
- Test command validation, stale-command timeouts, disconnect behavior, RC failsafe behavior, and safe stop behavior without relying only on a successful dashboard connection.
- Use hardware-in-the-loop testing for GPIO, receiver, GPS, camera, and Roboteq integration before deployment.
- Document Pi setup, serial-device configuration, camera endpoints, service management, and recovery procedures.
- Do not infer that a component has migrated to this architecture until its implementation and deployment configuration have actually changed.
