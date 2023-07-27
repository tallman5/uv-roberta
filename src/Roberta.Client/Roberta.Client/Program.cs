using Microsoft.AspNetCore.SignalR.Client;
using Microsoft.Extensions.Configuration;
using Roberta;
using Roberta.Io;
using System.ComponentModel;

bool keepRunning = true;
Console.Clear();
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

var result = Utilities.GetClientToken(tenantId, clientId, configuration["RobertaClientSecret"], scope);
var token = result.AccessToken;

// Hub Connection
HubConnection hubConnection;
Console.Write("Connecting to hub...");
hubConnection = new HubConnectionBuilder()
    .WithUrl(hubUrl, options =>
    {
        options.AccessTokenProvider = async () =>
        {
            return await Task.FromResult(token);
        };
    })
    .WithAutomaticReconnect()
    .Build();
await hubConnection.StartAsync();
Console.WriteLine("done.");

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

Console.WriteLine("Running, press Ctrl+C to exit");
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
