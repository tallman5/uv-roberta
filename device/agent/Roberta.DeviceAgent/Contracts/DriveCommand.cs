namespace Roberta.DeviceAgent.Contracts;

public sealed record DriveCommand(
    int ProtocolVersion,
    Guid CommandId,
    double Throttle,
    double Steering,
    DateTimeOffset ExpiresAt);

public sealed record CommandAcknowledgement(
    int ProtocolVersion,
    Guid CommandId,
    bool Accepted,
    string Detail);

public sealed record GpsFix(
    double Latitude,
    double Longitude,
    DateTimeOffset ObservedAt,
    double? AccuracyMeters);

public sealed record CameraStream(string Name, string Url);

public sealed record RobertaTelemetry(
    int ProtocolVersion,
    string DeviceId,
    DateTimeOffset ObservedAt,
    string ControlSource,
    bool SlowMode,
    bool MotorEnabled,
    string? Fault,
    GpsFix? Gps,
    IReadOnlyList<CameraStream> CameraStreams);
