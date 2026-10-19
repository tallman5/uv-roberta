export interface DriveCommand {
  protocolVersion: 1;
  commandId: string;
  throttle: number;
  steering: number;
  expiresAt: string;
}

export interface StickBounds {
  left: number;
  top: number;
  width: number;
  height: number;
}

const clamp = (value: number) =>
  Number.isFinite(value) ? Math.max(-1, Math.min(1, value)) : 0;

export function mapThumbstick(
  clientX: number,
  clientY: number,
  bounds: StickBounds,
): { throttle: number; steering: number } {
  if (bounds.width <= 0 || bounds.height <= 0) {
    return { throttle: 0, steering: 0 };
  }

  return {
    steering: clamp((clientX - (bounds.left + bounds.width / 2)) / (bounds.width / 2)),
    throttle: clamp(((bounds.top + bounds.height / 2) - clientY) / (bounds.height / 2)),
  };
}

export function createDriveCommand(
  throttle: number,
  steering: number,
  nowMilliseconds: number,
  commandId: string,
): DriveCommand {
  return {
    protocolVersion: 1,
    commandId,
    throttle: clamp(throttle),
    steering: clamp(steering),
    expiresAt: new Date(nowMilliseconds + 450).toISOString(),
  };
}
