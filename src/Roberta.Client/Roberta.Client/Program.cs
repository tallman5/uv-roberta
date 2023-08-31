using Microsoft.AspNetCore.SignalR.Client;
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
var roboteqPortName = "/dev/ttyACM0";
var lastRoboteqTs = DateTimeOffset.MinValue;

#if DEBUG
leftConnection = "COM5";
hubUrl = "https://localhost:7224/robertaHub";
roboteqPortName = "COM5";
#endif

// Auth
var token = string.Empty;
Console.Write("Aquiring access token...");
var result = Utilities.GetClientToken(
    configuration["AzureAD:TenantId"],
    configuration["DeviceClientId"], 
    configuration["DeviceSecret"],
    configuration["Scope"]);
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
        Console.WriteLine(ex);
        Console.WriteLine("Waiting ten seconds to try again.");
        Thread.Sleep(10000);
    }
}

var connections = new List<IConnection>();

var leftGpsState = new GpsState { Title = "Left GPS" };
var leftGpsConnection = new GpsConnection(leftConnection, leftGpsState);
connections.Add(leftGpsConnection);

var roboteqState = new RoboteqState { Title = "Roboteq" };
var roboteqConnection = new RoboteqConnection(roboteqPortName, roboteqState);
connections.Add(roboteqConnection);
hubConnection.On<decimal, decimal>("SetXY", (x, y) =>
{
    SetXY(x, y);
});

foreach (var connection in connections)
{
    Console.Write($"Opening {connection.ConnectionString} connection...");
    connection.Open();
    Console.WriteLine("done.");
}

leftGpsState.PropertyChanged += LeftGpsState_PropertyChanged;
roboteqState.PropertyChanged += RoboteqState_PropertyChanged;

while (keepRunning)
{
    Thread.Sleep(1000);
}

Console.WriteLine("Cleaning up resources...");
await hubConnection.StopAsync();
await hubConnection.DisposeAsync();
foreach (var connection in connections)
{
    connection.Dispose();
}

void Console_CancelKeyPress(object? sender, ConsoleCancelEventArgs e)
{
    e.Cancel = true;
    keepRunning = false;
}

async void LeftGpsState_PropertyChanged(object? sender, PropertyChangedEventArgs e)
{
    if (leftGpsState.Timestamp > lastGpsTs)
    {
        await SendToHubAsync("UpdateGpsState", leftGpsState);
        lastGpsTs = leftGpsState.Timestamp;
    }
}

async void RoboteqState_PropertyChanged(object? sender, PropertyChangedEventArgs e)
{
    if (roboteqState.Timestamp > lastRoboteqTs)
    {
        await SendToHubAsync("UpdateRoboteqState", roboteqState);
        lastRoboteqTs = roboteqState.Timestamp;
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

void SetXY(decimal x, decimal y)
{
    string newLine = $"!M {y}, {x}";
    roboteqConnection?.Send(newLine);
}
