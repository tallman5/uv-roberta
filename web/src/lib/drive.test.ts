import { describe, expect, it } from "vitest";
import { createDriveCommand, mapThumbstick } from "./drive";

describe("virtual thumbstick mapping", () => {
  const bounds = { left: 10, top: 20, width: 200, height: 200 };

  it("maps center to neutral", () => {
    expect(mapThumbstick(110, 120, bounds)).toEqual({ throttle: 0, steering: 0 });
  });

  it("maps edges to normalized drive input", () => {
    expect(mapThumbstick(210, 20, bounds)).toEqual({ throttle: 1, steering: 1 });
    expect(mapThumbstick(10, 220, bounds)).toEqual({ throttle: -1, steering: -1 });
  });

  it("clamps points outside the stick", () => {
    expect(mapThumbstick(500, -100, bounds)).toEqual({ throttle: 1, steering: 1 });
  });

  it("returns neutral for unmeasured bounds", () => {
    expect(mapThumbstick(0, 0, { ...bounds, width: 0 })).toEqual({
      throttle: 0,
      steering: 0,
    });
  });
});

describe("drive command creation", () => {
  it("clamps input and gives commands a short expiry", () => {
    const now = Date.parse("2026-01-01T00:00:00.000Z");

    expect(createDriveCommand(2, -2, now, "command-id")).toEqual({
      protocolVersion: 1,
      commandId: "command-id",
      throttle: 1,
      steering: -1,
      expiresAt: "2026-01-01T00:00:00.450Z",
    });
  });
});
