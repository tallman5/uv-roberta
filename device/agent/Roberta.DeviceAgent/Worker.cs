using System.Net.Http;
using Microsoft.AspNetCore.SignalR;
using Microsoft.AspNetCore.SignalR.Client;
using Roberta.DeviceAgent.Contracts;

namespace Roberta.DeviceAgent;

public sealed class Worker(
    IConfiguration configuration,
    ILocalControl localControl,
    ILogger<Worker> logger) : BackgroundService
{
    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        var hubUrl = configuration["DeviceAgent:HubUrl"];
        var accessToken = configuration["DeviceAgent:AccessToken"];
        if (string.IsNullOrWhiteSpace(hubUrl) || string.IsNullOrWhiteSpace(accessToken))
        {
            throw new InvalidOperationException(
                "DeviceAgent:HubUrl and DeviceAgent:AccessToken must be configured.");
        }

        var hub = new HubConnectionBuilder()
            .WithUrl(hubUrl, options => options.AccessTokenProvider = () => Task.FromResult<string?>(accessToken))
            .WithAutomaticReconnect()
            .Build();

        hub.On<DriveCommand>("DriveCommand", async command =>
        {
            var acknowledgement = await localControl.ApplyAsync(command, stoppingToken);
            if (hub.State == HubConnectionState.Connected)
            {
                await hub.InvokeAsync("AcknowledgeCommand", acknowledgement, stoppingToken);
            }
        });

        hub.Reconnecting += exception =>
        {
            logger.LogWarning(exception, "SignalR connection lost; the local controller must stop on stale input.");
            return Task.CompletedTask;
        };
        hub.Reconnected += connectionId =>
        {
            logger.LogInformation("Device agent reconnected to the hub.");
            return Task.CompletedTask;
        };

        while (!stoppingToken.IsCancellationRequested)
        {
            if (hub.State == HubConnectionState.Disconnected)
            {
                try
                {
                    await hub.StartAsync(stoppingToken);
                    logger.LogInformation("Device agent connected to the hub.");
                }
                catch (HttpRequestException exception)
                {
                    logger.LogWarning(exception, "Unable to connect to the hub; retrying.");
                    await Task.Delay(TimeSpan.FromSeconds(2), stoppingToken);
                    continue;
                }
                catch (HubException exception)
                {
                    logger.LogError(exception, "The hub rejected the device connection.");
                    await Task.Delay(TimeSpan.FromSeconds(10), stoppingToken);
                    continue;
                }
            }

            if (hub.State == HubConnectionState.Connected)
            {
                var telemetry = await localControl.ReadTelemetryAsync(stoppingToken);
                try
                {
                    await hub.InvokeAsync("PublishTelemetry", telemetry, stoppingToken);
                }
                catch (InvalidOperationException exception)
                {
                    logger.LogWarning(exception, "Telemetry could not be sent because the hub connection changed.");
                }
            }

            await Task.Delay(TimeSpan.FromSeconds(1), stoppingToken);
        }

        await hub.StopAsync(CancellationToken.None);
        await hub.DisposeAsync();
    }
}
