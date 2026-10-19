using System.Collections.Concurrent;
using Roberta.Api.Contracts;

namespace Roberta.Api.Services;

public sealed class PendingCommandRegistry
{
    private readonly ConcurrentDictionary<Guid, PendingCommand> commands = new();

    public PendingCommand Register(Guid commandId, string deviceConnectionId)
    {
        var pending = new PendingCommand(deviceConnectionId);
        if (!commands.TryAdd(commandId, pending))
        {
            throw new InvalidOperationException("A command with this ID is already pending.");
        }

        return pending;
    }

    public bool TryAcknowledge(string deviceConnectionId, CommandAcknowledgement acknowledgement)
    {
        if (!commands.TryGetValue(acknowledgement.CommandId, out var pending) ||
            pending.DeviceConnectionId != deviceConnectionId)
        {
            return false;
        }

        return pending.Completion.TrySetResult(acknowledgement);
    }

    public void Remove(Guid commandId, PendingCommand pending)
    {
        commands.TryRemove(new KeyValuePair<Guid, PendingCommand>(commandId, pending));
    }

    public sealed class PendingCommand(string deviceConnectionId)
    {
        internal TaskCompletionSource<CommandAcknowledgement> Completion { get; } =
            new(TaskCreationOptions.RunContinuationsAsynchronously);

        public string DeviceConnectionId { get; } = deviceConnectionId;

        public Task<CommandAcknowledgement> Task => Completion.Task;
    }
}
