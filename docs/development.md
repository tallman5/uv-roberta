# Later-phase development and deployment

This guide is for the API/dashboard/device-agent phases. It is not part of the Phase 1 RC-only startup procedure; follow [the Phase 1 setup guide](phase1-setup.md) to run the robot.

## Component boundaries

Build and deploy the dashboard, API, device agent, and native control code independently. The API is an ASP.NET Core 9 SignalR service; it is not hosted by the static Qwik dashboard. The Pi-side agent uses a device-scoped JWT and exchanges versioned control/telemetry messages with the hub. The hub carries GPS metadata and camera stream URLs, not camera video. The C++ library contains the device-side source arbitration, command freshness checks, motor mixing, and slow-mode limit.

The current `UnconfiguredLocalControl` implementation in the device agent intentionally rejects every drive command and reports `motorEnabled: false`. Replace it only after the hardware-specific receiver and motor adapters have been implemented and verified. Do not deploy the current agent as a driving robot.

## API configuration

Set values in the API process environment or a secret manager:

```text
Authentication__Authority=https://your-oidc-issuer/
Authentication__Audience=uv-roberta-api
Hub__DeviceId=roberta-01
Cors__AllowedOrigins__0=https://your-dashboard.example
```

The API refuses to start without an authority, audience, and at least one exact dashboard origin. JWTs must have `role=operator` for dashboard users. Device JWTs must have `role=device` and `device_id` equal to `Hub:DeviceId`. Issue operator and device credentials separately with narrow audience/scope, expiry, and revocation policy.

The SignalR browser transport may send its bearer token in the `access_token` query parameter during WebSocket negotiation. Configure the API host, reverse proxy, and observability pipeline to redact that parameter from request logs.

For local development, `api/Roberta.Api/appsettings.Development.json` allows the Qwik dev origin `http://localhost:5173`. Configure the remaining authentication values through the environment; do not add a token or signing key to appsettings.

## Device-agent configuration

Use environment configuration or .NET User Secrets for development:

```powershell
dotnet user-secrets set "DeviceAgent:HubUrl" "https://localhost:7001/hubs/roberta" --project device\agent\Roberta.DeviceAgent
dotnet user-secrets set "DeviceAgent:AccessToken" "<device-scoped JWT>" --project device\agent\Roberta.DeviceAgent
```

`Hub:DeviceId` defaults to `roberta-01` in the checked-in appsettings and must match the API's device ID and the device token claim.

## Local startup

Run each component in a separate terminal after supplying API authentication configuration:

```powershell
dotnet run --project api\Roberta.Api --urls https://localhost:7001
dotnet run --project device\agent\Roberta.DeviceAgent
Set-Location web
npm run dev
```

The dashboard starts without an access token. Enter an operator JWT at runtime; it remains in memory and is not saved in local or session storage. The dashboard shows a command as acknowledged only after the device agent returns an acknowledgement. `GET /healthz` reports API process health, not robot or hardware health.

The device agent and API need a reachable HTTPS endpoint and a correctly configured JWT issuer. Before the hardware adapter is configured, the device agent will report the hardware fault and reject commands.

## Verification

```powershell
dotnet test Roberta.sln
cmake -S device\control -B build\control
cmake --build build\control
ctest --test-dir build\control --output-on-failure
Set-Location web
npm ci
npm test
npm run build
```

Native unit tests are hardware-free policy tests. They do not validate RC wiring, motor direction, USB serial timing, GPIO failsafe behavior, or the physical emergency stop.
