using Roberta.DeviceAgent.Contracts;

namespace Roberta.DeviceAgent;

public sealed class UnconfiguredLocalControl : ILocalControl
{
    private const string Fault = "Local receiver and motor adapters are not configured; motor output is disabled.";
    private readonly string deviceId;

    public UnconfiguredLocalControl(
        IConfiguration configuration,
        ILogger<UnconfiguredLocalControl> logger)
    {
        deviceId = configuration["Hub:DeviceId"] ?? "roberta-01";
        logger.LogCritical("{Fault}", Fault);
    }

    public ValueTask<CommandAcknowledgement> ApplyAsync(
        DriveCommand command,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        return ValueTask.FromResult(new CommandAcknowledgement(
            1,
            command.CommandId,
            false,
            Fault));
    }

    public ValueTask<RobertaTelemetry> ReadTelemetryAsync(CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        return ValueTask.FromResult(new RobertaTelemetry(
            1,
            deviceId,
            DateTimeOffset.UtcNow,
            "unknown",
            false,
            false,
            Fault,
            null,
            Array.Empty<CameraStream>()));
    }
}
