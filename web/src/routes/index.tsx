import {
  $,
  component$,
  noSerialize,
  useSignal,
  useStore,
  useVisibleTask$,
  type NoSerialize,
} from "@builder.io/qwik";
import {
  HubConnectionBuilder,
  HubConnectionState,
  LogLevel,
  type HubConnection,
} from "@microsoft/signalr";
import { createDriveCommand, mapThumbstick } from "../lib/drive";

interface CommandAcknowledgement {
  protocolVersion: number;
  commandId: string;
  accepted: boolean;
  detail: string;
}

interface GpsFix {
  latitude: number;
  longitude: number;
  observedAt: string;
  accuracyMeters?: number | null;
}

interface CameraStream {
  name: string;
  url: string;
}

interface RobertaTelemetry {
  protocolVersion: number;
  deviceId: string;
  observedAt: string;
  controlSource: "rc" | "dashboard" | "unknown";
  slowMode: boolean;
  motorEnabled: boolean;
  fault: string | null;
  gps: GpsFix | null;
  cameraStreams: CameraStream[];
}

export default component$(() => {
  const state = useStore({
    hubUrl: "https://localhost:7001/hubs/roberta",
    token: "",
    connecting: false,
    connected: false,
    connectionStatus: "Disconnected",
    acknowledgement: "No command sent",
    telemetry: undefined as RobertaTelemetry | undefined,
    telemetryFresh: false,
    throttle: 0,
    steering: 0,
    dragging: false,
    sending: false,
    error: "",
  });
  const connection = useSignal<NoSerialize<HubConnection>>();
  const stick = useSignal<HTMLDivElement>();

  const sendDrive = $(async (throttle: number, steering: number) => {
    const hub = connection.value;
    if (
      !hub ||
      hub.state !== HubConnectionState.Connected ||
      !state.telemetryFresh ||
      state.telemetry?.controlSource !== "dashboard" ||
      state.sending
    ) {
      return;
    }

    state.sending = true;
    const command = createDriveCommand(
      throttle,
      steering,
      Date.now(),
      crypto.randomUUID(),
    );
    try {
      const acknowledgement =
        await hub.invoke<CommandAcknowledgement>("SendDriveCommand", command);
      state.acknowledgement = acknowledgement.accepted
        ? `Device acknowledged command ${acknowledgement.commandId}`
        : `Device rejected command: ${acknowledgement.detail}`;
    } catch (error) {
      state.acknowledgement =
        error instanceof Error ? `Command failed: ${error.message}` : "Command failed";
      state.throttle = 0;
      state.steering = 0;
    } finally {
      state.sending = false;
    }
  });

  const sendNeutral = $(async () => {
    const hub = connection.value;
    if (!hub || hub.state !== HubConnectionState.Connected) {
      return;
    }

    try {
      const acknowledgement = await hub.invoke<CommandAcknowledgement>(
        "SendDriveCommand",
        createDriveCommand(0, 0, Date.now(), crypto.randomUUID()),
      );
      state.acknowledgement = acknowledgement.accepted
        ? "Device acknowledged neutral command"
        : `Neutral command rejected: ${acknowledgement.detail}`;
    } catch (error) {
      state.acknowledgement =
        error instanceof Error ? `Neutral command failed: ${error.message}` : "Neutral command failed";
    }
  });

  const connect = $(async () => {
    if (!state.hubUrl.trim() || !state.token.trim()) {
      state.error = "Enter the hub URL and an operator access token.";
      return;
    }

    state.error = "";
    state.connecting = true;
    const hub = new HubConnectionBuilder()
      .withUrl(state.hubUrl.trim(), {
        accessTokenFactory: () => state.token,
      })
      .withAutomaticReconnect()
      .configureLogging(LogLevel.Warning)
      .build();

    hub.on("Telemetry", (telemetry: RobertaTelemetry) => {
      state.telemetry = telemetry;
      state.telemetryFresh =
        Date.now() - Date.parse(telemetry.observedAt) <= 2000;
      if (telemetry.controlSource !== "dashboard") {
        state.throttle = 0;
        state.steering = 0;
      }
    });
    hub.onreconnecting(() => {
      state.connected = false;
      state.connectionStatus = "Reconnecting";
    });
    hub.onreconnected(() => {
      state.connected = true;
      state.connectionStatus = "Connected";
    });
    hub.onclose(() => {
      state.connected = false;
      state.connectionStatus = "Disconnected";
      state.throttle = 0;
      state.steering = 0;
    });

    try {
      await hub.start();
      connection.value = noSerialize(hub);
      state.connected = true;
      state.connectionStatus = "Connected";
    } catch (error) {
      state.connectionStatus = "Connection failed";
      state.error =
        error instanceof Error ? error.message : "Unable to connect to the hub.";
    } finally {
      state.connecting = false;
    }
  });

  const disconnect = $(async () => {
    const hub = connection.value;
    connection.value = undefined;
    await hub?.stop();
    state.connected = false;
    state.connectionStatus = "Disconnected";
    state.telemetryFresh = false;
    state.throttle = 0;
    state.steering = 0;
  });

  const updateStick = $((clientX: number, clientY: number) => {
    const bounds = stick.value?.getBoundingClientRect();
    if (!bounds) {
      return;
    }

    const input = mapThumbstick(clientX, clientY, bounds);
    state.steering = input.steering;
    state.throttle = input.throttle;
  });

  // The browser timer sends periodic command heartbeats and cannot run during SSR.
  // eslint-disable-next-line qwik/no-use-visible-task
  useVisibleTask$(() => {
    const timer = window.setInterval(() => {
      const observedAt = state.telemetry?.observedAt;
      state.telemetryFresh =
        observedAt !== undefined && Date.now() - Date.parse(observedAt) <= 2000;
      if (
        state.connected &&
        state.telemetryFresh &&
        state.telemetry?.controlSource === "dashboard"
      ) {
        void sendDrive(state.throttle, state.steering);
      } else {
        state.throttle = 0;
        state.steering = 0;
      }
    }, 100);

    return () => window.clearInterval(timer);
  });

  const dashboardHasControl =
    state.connected &&
    state.telemetryFresh &&
    state.telemetry?.controlSource === "dashboard";

  return (
    <main class="container py-4 py-lg-5">
      <header class="d-flex flex-wrap align-items-center justify-content-between gap-3 mb-4">
        <div>
          <p class="text-uppercase text-primary fw-semibold small mb-1">UV Roberta v2</p>
          <h1 class="h2 mb-0">Operator dashboard</h1>
        </div>
        <span class={`badge ${state.connected ? "text-bg-success" : "text-bg-secondary"}`}>
          {state.connectionStatus}
        </span>
      </header>

      <section class="card shadow-sm mb-4" aria-labelledby="connection-heading">
        <div class="card-body">
          <h2 id="connection-heading" class="h5">Secure connection</h2>
          <div class="row g-3 align-items-end">
            <div class="col-12 col-lg-5">
              <label for="hub-url" class="form-label">Hub URL</label>
              <input
                id="hub-url"
                class="form-control"
                type="url"
                value={state.hubUrl}
                onInput$={(_, target) => (state.hubUrl = target.value)}
                disabled={state.connected || state.connecting}
              />
            </div>
            <div class="col-12 col-lg-5">
              <label for="operator-token" class="form-label">Operator access token</label>
              <input
                id="operator-token"
                class="form-control"
                type="password"
                autocomplete="off"
                value={state.token}
                onInput$={(_, target) => (state.token = target.value)}
                disabled={state.connected || state.connecting}
              />
              <div class="form-text">Kept in memory only; it is not saved in browser storage.</div>
            </div>
            <div class="col-12 col-lg-2 d-grid">
              {state.connected ? (
                <button class="btn btn-outline-secondary" onClick$={disconnect}>
                  Disconnect
                </button>
              ) : (
                <button
                  class="btn btn-primary"
                  onClick$={connect}
                  disabled={state.connecting}
                >
                  {state.connecting ? "Connecting..." : "Connect"}
                </button>
              )}
            </div>
          </div>
          {state.error && <p class="alert alert-danger mt-3 mb-0" role="alert">{state.error}</p>}
        </div>
      </section>

      <div class="row g-4">
        <section class="col-12 col-lg-7" aria-labelledby="control-heading">
          <div class="card shadow-sm h-100">
            <div class="card-body d-flex flex-column align-items-center">
              <h2 id="control-heading" class="h5 align-self-start">Virtual thumbstick</h2>
              <p class="text-body-secondary align-self-start">
                Dashboard drive is enabled only when the RC priority switch selects dashboard.
              </p>
              <div
                ref={stick}
                class={`virtual-stick ${dashboardHasControl ? "" : "virtual-stick--disabled"}`}
                role="application"
                aria-label="Virtual thumbstick. Drag to steer and drive."
                aria-disabled={!dashboardHasControl}
                onPointerDown$={(event) => {
                  if (!dashboardHasControl) return;
                  state.dragging = true;
                  stick.value?.setPointerCapture(event.pointerId);
                  updateStick(event.clientX, event.clientY);
                }}
                onPointerMove$={(event) => {
                  if (state.dragging && dashboardHasControl) {
                    updateStick(event.clientX, event.clientY);
                  }
                }}
                onPointerUp$={() => {
                  state.dragging = false;
                  state.throttle = 0;
                  state.steering = 0;
                  void sendNeutral();
                }}
                onPointerCancel$={() => {
                  state.dragging = false;
                  state.throttle = 0;
                  state.steering = 0;
                  void sendNeutral();
                }}
              >
                <span
                  class="virtual-stick__knob"
                  style={`transform: translate(${state.steering * 54}px, ${-state.throttle * 54}px)`}
                />
              </div>
              <div class="d-flex gap-4 mt-3" aria-live="polite">
                <span>Throttle: {state.throttle.toFixed(2)}</span>
                <span>Steering: {state.steering.toFixed(2)}</span>
              </div>
              <button
                class="btn btn-outline-danger mt-3"
                disabled={!dashboardHasControl}
                onClick$={() => {
                  state.throttle = 0;
                  state.steering = 0;
                  void sendNeutral();
                }}
              >
                Send neutral command
              </button>
            </div>
          </div>
        </section>

        <section class="col-12 col-lg-5" aria-labelledby="status-heading">
          <div class="card shadow-sm h-100">
            <div class="card-body">
              <h2 id="status-heading" class="h5">Robot status</h2>
              {state.telemetry && state.telemetryFresh ? (
                <dl class="row mb-0">
                  <dt class="col-6">Device</dt>
                  <dd class="col-6">{state.telemetry.deviceId}</dd>
                  <dt class="col-6">Control source</dt>
                  <dd class="col-6">{state.telemetry.controlSource}</dd>
                  <dt class="col-6">Slow mode</dt>
                  <dd class="col-6">{state.telemetry.slowMode ? "On (25%)" : "Off"}</dd>
                  <dt class="col-6">Motor output</dt>
                  <dd class="col-6">{state.telemetry.motorEnabled ? "Enabled" : "Stopped"}</dd>
                  <dt class="col-6">Fault</dt>
                  <dd class="col-6">{state.telemetry.fault ?? "None reported"}</dd>
                  <dt class="col-6">GPS</dt>
                  <dd class="col-6">
                    {state.telemetry.gps
                      ? `${state.telemetry.gps.latitude.toFixed(6)}, ${state.telemetry.gps.longitude.toFixed(6)}`
                      : "No fix"}
                  </dd>
                  <dt class="col-6">Telemetry received</dt>
                  <dd class="col-6">{new Date(state.telemetry.observedAt).toLocaleTimeString()}</dd>
                </dl>
              ) : (
                <p class="alert alert-warning mb-0" role="status">
                  Telemetry is unavailable or stale. Dashboard commands are disabled.
                </p>
              )}
              {state.telemetryFresh && state.telemetry?.cameraStreams.length ? (
                <div class="mt-3">
                  <h3 class="h6">Camera streams</h3>
                  <ul class="mb-0">
                    {state.telemetry.cameraStreams.map((camera) => (
                      <li key={camera.name}>
                        <a href={camera.url} target="_blank" rel="noopener noreferrer">
                          {camera.name} (open stream)
                        </a>
                      </li>
                    ))}
                  </ul>
                </div>
              ) : null}
              <p class="small mt-3 mb-0" aria-live="polite">{state.acknowledgement}</p>
            </div>
          </div>
        </section>
      </div>

      <p class="small text-body-secondary mt-4 mb-0">
        Local RC failsafe and motor limits remain authoritative. This dashboard is not an emergency stop.
      </p>
    </main>
  );
});
