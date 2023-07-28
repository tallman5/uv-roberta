﻿using Microsoft.AspNetCore.SignalR.Client;
using Microsoft.Extensions.Configuration;
using Roberta;
using Roberta.Io;
using System.ComponentModel;

bool keepRunning = true;
Console.Clear();
Console.WriteLine("Running, press Ctrl+C to exit");

Console.CancelKeyPress += Console_CancelKeyPress;
IConfiguration configuration = new ConfigurationBuilder()
            .AddJsonFile("secrets.json")
            .Build();

var ipAddress = Utilities.GetLocalIPAddress();
var hubUrl = $"https://rofo.mcgurkin.net:5001/robertaHub";
string leftConnection = "/dev/serial/by-id/usb-Prolific_Technology_Inc._USB-Serial_Controller_D-if00-port0";
var lastGpsTs = DateTimeOffset.MinValue;

#if DEBUG
leftConnection = "COM5";
//hubUrl = "https://localhost:7224/robertaHub";
#endif

// Auth
string tenantId = "a2716cd4-06bd-4131-a023-1a69bfae111a";
string clientId = "499964bd-21a3-4135-85ed-1456fd18c186";
string scope = "api://742cd12c-d936-49ea-8ea6-de4f3a785aad/.default";

var token = string.Empty;
Console.Write("Aquiring access token...");
var result = Utilities.GetClientToken(tenantId, clientId, configuration["RobertaClientSecret"], scope);
token = result.AccessToken;
Console.WriteLine("done.");

// Hub Connection
HubConnection hubConnection = new HubConnectionBuilder()
    .WithUrl(hubUrl, options =>
    {
        options.AccessTokenProvider = async () =>
        {
            return await Task.FromResult(token);
        };
    })
    .WithAutomaticReconnect()
    .Build();
while (hubConnection.State != HubConnectionState.Connected && keepRunning)
{
    try
    {
        Console.Write("Connecting to hub...");
        await hubConnection.StartAsync();
        Console.WriteLine("done.");
    }
    catch (Exception ex)
    {
        Console.WriteLine("error.");
        Console.WriteLine(ex.Message);
        Console.WriteLine("Waiting ten seconds to try again.");
        Thread.Sleep(10000);
    }
}

var connections = new List<IConnection>();
var leftGpsState = new GpsState { Title = "Left GPS" };
var leftGpsConnection = new GpsConnection(leftConnection, leftGpsState);
connections.Add(leftGpsConnection);

foreach (var connection in connections)
{
    Console.Write($"Opening {connection.ConnectionString} connection...");
    connection.Open();
    Console.WriteLine("done.");
}

leftGpsState.PropertyChanged += LeftGpsState_PropertyChanged;

while (keepRunning)
{
    Thread.Sleep(1000);
}

Console.WriteLine("Cleaning up resources...");
await hubConnection.StopAsync();
await hubConnection.DisposeAsync();

async void LeftGpsState_PropertyChanged(object? sender, PropertyChangedEventArgs e)
{
    if (leftGpsState.Timestamp > lastGpsTs)
    {
        await SendToHubAsync("UpdateGpsState", leftGpsState);
        lastGpsTs = leftGpsState.Timestamp;
    }
}

async Task SendToHubAsync(string methodName, object arg1)
{
    if (hubConnection.State == HubConnectionState.Connected)
    {
        try
        {
            await hubConnection.InvokeAsync(methodName, arg1);
        }
        catch (Exception ex)
        {
            Console.WriteLine(ex);
        }
    }
}

void Console_CancelKeyPress(object? sender, ConsoleCancelEventArgs e)
{
    e.Cancel = true;
    keepRunning = false;
}
