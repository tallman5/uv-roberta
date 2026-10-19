using Roberta.DeviceAgent.Contracts;

namespace Roberta.DeviceAgent;

public interface ILocalControl
{
    ValueTask<CommandAcknowledgement> ApplyAsync(
        DriveCommand command,
        CancellationToken cancellationToken);

    ValueTask<RobertaTelemetry> ReadTelemetryAsync(CancellationToken cancellationToken);
}
