using Roberta.Api.Contracts;
using Roberta.Api.Services;

namespace Roberta.Api.Tests;

public sealed class DeviceConnectionRegistryTests
{
    [Fact]
    public void AllowsOnlyOneActiveDeviceConnectionAndUnregistersByConnectionId()
    {
        var registry = new DeviceConnectionRegistry();

        Assert.True(registry.Register("connection-a"));
        Assert.False(registry.Register("connection-b"));

        registry.Unregister("connection-b");
        Assert.Equal("connection-a", registry.GetConnectionId());

        registry.Unregister("connection-a");
        Assert.Null(registry.GetConnectionId());
        Assert.True(registry.Register("connection-b"));
    }

    [Fact]
    public async Task AcknowledgementMustComeFromThePendingDeviceConnection()
    {
        var registry = new PendingCommandRegistry();
        var id = Guid.NewGuid();
        var pending = registry.Register(id, "device-a");
        var acknowledgement = new CommandAcknowledgement(1, id, true, "applied");

        Assert.False(registry.TryAcknowledge("device-b", acknowledgement));
        Assert.False(pending.Task.IsCompleted);
        Assert.True(registry.TryAcknowledge("device-a", acknowledgement));
        Assert.Equal(acknowledgement, await pending.Task);
        registry.Remove(id, pending);
    }
}
