using Roberta.Api.Contracts;

namespace Roberta.Api.Services;

public static class TelemetryValidator
{
    public static string? Validate(
        RobertaTelemetry? telemetry,
        string expectedDeviceId,
        DateTimeOffset now)
    {
        if (telemetry is null ||
            telemetry.ProtocolVersion != 1 ||
            telemetry.DeviceId != expectedDeviceId ||
            telemetry.ControlSource is not ("rc" or "dashboard" or "unknown") ||
            telemetry.ObservedAt > now.AddSeconds(1) ||
            telemetry.Fault?.Length > 256)
        {
            return "Telemetry is invalid.";
        }

        if (telemetry.Gps is { } gps &&
            (!double.IsFinite(gps.Latitude) ||
             gps.Latitude is < -90 or > 90 ||
             !double.IsFinite(gps.Longitude) ||
             gps.Longitude is < -180 or > 180 ||
             gps.ObservedAt > now.AddSeconds(1) ||
             (gps.AccuracyMeters is { } accuracy &&
              (!double.IsFinite(accuracy) || accuracy < 0))))
        {
            return "GPS telemetry is invalid.";
        }

        if (telemetry.CameraStreams is null ||
            telemetry.CameraStreams.Count > 8 ||
            telemetry.CameraStreams.Any(stream =>
                stream is null ||
                string.IsNullOrWhiteSpace(stream.Name) ||
                stream.Name.Length > 64 ||
                !Uri.TryCreate(stream.Url, UriKind.Absolute, out var uri) ||
                (uri.Scheme is not ("https" or "http")) ||
                !string.IsNullOrEmpty(uri.UserInfo)))
        {
            return "Camera stream metadata is invalid.";
        }

        return null;
    }
}
