using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.Logging.Abstractions;
using Roberta.DeviceAgent;
using Roberta.DeviceAgent.Contracts;

namespace Roberta.DeviceAgent.Tests;

public sealed class UnconfiguredLocalControlTests
{
    [Fact]
    public async Task RejectsDriveCommandsUntilHardwareAdaptersAreConfigured()
    {
        var configuration = new ConfigurationBuilder()
            .AddInMemoryCollection(new Dictionary<string, string?> { ["Hub:DeviceId"] = "roberta-test" })
            .Build();
        var localControl = new UnconfiguredLocalControl(
            configuration,
            NullLogger<UnconfiguredLocalControl>.Instance);
        var command = new DriveCommand(1, Guid.NewGuid(), 0.5, 0, DateTimeOffset.UtcNow.AddMilliseconds(450));

        var acknowledgement = await localControl.ApplyAsync(command, CancellationToken.None);

        Assert.Equal(command.CommandId, acknowledgement.CommandId);
        Assert.False(acknowledgement.Accepted);
        Assert.Contains("motor output is disabled", acknowledgement.Detail);
    }

    [Fact]
    public async Task ReportsUnknownSourceAndDisabledMotorsUntilHardwareIsConfigured()
    {
        var configuration = new ConfigurationBuilder()
            .AddInMemoryCollection(new Dictionary<string, string?> { ["Hub:DeviceId"] = "roberta-test" })
            .Build();
        var localControl = new UnconfiguredLocalControl(
            configuration,
            NullLogger<UnconfiguredLocalControl>.Instance);

        var telemetry = await localControl.ReadTelemetryAsync(CancellationToken.None);

        Assert.Equal("roberta-test", telemetry.DeviceId);
        Assert.Equal("unknown", telemetry.ControlSource);
        Assert.False(telemetry.MotorEnabled);
        Assert.NotNull(telemetry.Fault);
    }
}
