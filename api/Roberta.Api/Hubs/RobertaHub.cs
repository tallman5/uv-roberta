using System.Security.Claims;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.SignalR;
using Roberta.Api.Contracts;
using Roberta.Api.Services;

namespace Roberta.Api.Hubs;

[Authorize]
public sealed class RobertaHub(
    DeviceConnectionRegistry devices,
    PendingCommandRegistry pendingCommands,
    IConfiguration configuration,
    ILogger<RobertaHub> logger) : Hub
{
    private const string OperatorRole = "operator";
    private const string DeviceRole = "device";
    private const string DeviceIdClaim = "device_id";
    private static readonly TimeSpan AcknowledgementTimeout = TimeSpan.FromMilliseconds(750);

    private string DeviceId => configuration["Hub:DeviceId"] ?? "roberta-01";

    public override async Task OnConnectedAsync()
    {
        var principal = Context.User;
        if (principal?.IsInRole(DeviceRole) == true)
        {
            var claimedDeviceId = principal.FindFirstValue(DeviceIdClaim);
            if (claimedDeviceId != DeviceId || !devices.Register(Context.ConnectionId))
            {
                Context.Abort();
                return;
            }
        }
        else if (principal?.IsInRole(OperatorRole) == true)
        {
            await Groups.AddToGroupAsync(Context.ConnectionId, OperatorRole);
        }
        else
        {
            Context.Abort();
            return;
        }

        await base.OnConnectedAsync();
    }

    public override async Task OnDisconnectedAsync(Exception? exception)
    {
        devices.Unregister(Context.ConnectionId);
        await Groups.RemoveFromGroupAsync(Context.ConnectionId, OperatorRole);
        await base.OnDisconnectedAsync(exception);
    }

    public async Task<CommandAcknowledgement> SendDriveCommand(DriveCommand command)
    {
        EnsureRole(OperatorRole);
        var validationError = CommandValidator.Validate(command, DateTimeOffset.UtcNow);
        if (validationError is not null)
        {
            logger.LogWarning(
                "Rejected invalid drive command {CommandId} from operator {OperatorId}: {Reason}",
                command?.CommandId,
                Context.User?.FindFirstValue("sub") ?? "unknown",
                validationError);
            throw new HubException(validationError);
        }

        var deviceConnectionId = devices.GetConnectionId();
        if (deviceConnectionId is null)
        {
            throw new HubException("The robot device is offline.");
        }

        var pending = pendingCommands.Register(command.CommandId, deviceConnectionId);
        try
        {
            var operatorId = Context.User?.FindFirstValue("sub") ?? "unknown";
            await Clients.Client(deviceConnectionId).SendAsync("DriveCommand", command, Context.ConnectionAborted);
            var acknowledgement = await pending.Task.WaitAsync(AcknowledgementTimeout, Context.ConnectionAborted);
            logger.LogInformation(
                "Drive command {CommandId} from operator {OperatorId} received device acknowledgement: {Accepted}",
                command.CommandId,
                operatorId,
                acknowledgement.Accepted);
            return acknowledgement;
        }
        catch (TimeoutException exception)
        {
            logger.LogWarning(
                exception,
                "Drive command {CommandId} timed out waiting for a device acknowledgement.",
                command.CommandId);
            throw new HubException("The device did not acknowledge the command in time.");
        }
        finally
        {
            pendingCommands.Remove(command.CommandId, pending);
        }
    }

    public Task AcknowledgeCommand(CommandAcknowledgement? acknowledgement)
    {
        EnsureRole(DeviceRole);
        if (acknowledgement is null ||
            acknowledgement.ProtocolVersion != 1 ||
            acknowledgement.CommandId == Guid.Empty ||
            acknowledgement.Detail is null ||
            acknowledgement.Detail.Length > 256 ||
            !pendingCommands.TryAcknowledge(Context.ConnectionId, acknowledgement))
        {
            throw new HubException("The acknowledgement is invalid, unknown, or belongs to another device.");
        }

        return Task.CompletedTask;
    }

    public Task PublishTelemetry(RobertaTelemetry telemetry)
    {
        EnsureRole(DeviceRole);
        if (TelemetryValidator.Validate(telemetry, DeviceId, DateTimeOffset.UtcNow) is { } error)
        {
            throw new HubException(error);
        }

        return Clients.Group(OperatorRole).SendAsync("Telemetry", telemetry);
    }

    private void EnsureRole(string role)
    {
        if (Context.User?.IsInRole(role) != true)
        {
            throw new HubException("The caller is not authorized to perform this operation.");
        }
    }
}
