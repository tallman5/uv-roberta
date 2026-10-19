using Roberta.Api.Contracts;

namespace Roberta.Api.Services;

public static class CommandValidator
{
    public static string? Validate(DriveCommand? command, DateTimeOffset now)
    {
        if (command is null)
        {
            return "Command is required.";
        }

        if (command.ProtocolVersion != 1 || command.CommandId == Guid.Empty)
        {
            return "Unsupported protocol version or invalid command ID.";
        }

        if (!double.IsFinite(command.Throttle) ||
            !double.IsFinite(command.Steering) ||
            command.Throttle is < -1 or > 1 ||
            command.Steering is < -1 or > 1)
        {
            return "Drive values must be finite and between -1 and 1.";
        }

        if (command.ExpiresAt <= now || command.ExpiresAt > now.AddSeconds(1))
        {
            return "Command expiry must be in the future and no more than one second away.";
        }

        return null;
    }
}
