using Microsoft.AspNetCore.SignalR.Client;
using Roberta.Io;
using System.ComponentModel;

Console.Clear();

HubConnection hubConnection;
var handler = new HttpClientHandler
{
    // Ignore certificate validation errors
#if DEBUG
    ServerCertificateCustomValidationCallback = (sender, cert, chain, sslPolicyErrors) => true
#endif
};

string leftConnection = "/dev/serial/by-path/platform-fd500000.pcie-pci-0000:01:00.0-usb-0:1.1:1.0-port0";
string rightConnection = "/dev/serial/by-path/platform-fd500000.pcie-pci-0000:01:00.0-usb-0:1.2:1.0-port0";
string baseUrl = "https://192.168.1.156:7142";

#if DEBUG
leftConnection = "COM5";
rightConnection = "COM7";
baseUrl = "https://192.168.1.156:7142";
#endif

var connections = new List<Roberta.Io.IConnection>();

var leftGpsState = new GpsState { Title = "Left GPS" };
var leftGpsConnection = new GpsConnection(leftConnection, leftGpsState);
connections.Add(leftGpsConnection);

var rightGpsState = new GpsState { Title = "Right GPS" };
var rightGpsConnection = new GpsConnection(rightConnection, rightGpsState);
connections.Add(rightGpsConnection);

foreach (var connection in connections)
{
    Console.WriteLine($"Opening {connection.ConnectionString} connection...");
    connection.Open();
}

var gpsMixerState = new GpsMixerState(leftGpsState, rightGpsState);
gpsMixerState.Title = "GPS Mixer";

Console.WriteLine("Connecting to hub...");
var hubUrl = baseUrl + "/robertaHub";
hubConnection = new HubConnectionBuilder()
    .WithUrl(hubUrl, options =>
    {
        options.HttpMessageHandlerFactory = _ => handler;
    })
    .WithAutomaticReconnect()
    .Build();
await hubConnection.StartAsync();

Console.WriteLine("Reading GPS data...");
Console.WriteLine("Press any key to exit.");

gpsMixerState.PropertyChanged += GpsMixerState_Changed;

Console.ReadKey();

Console.WriteLine("Cleaning up...");

foreach (var connection in connections)
{
    Console.WriteLine($"Disposing {connection.ConnectionString} connection...");
    connection.Dispose();
}


void GpsMixerState_Changed(object? sender, PropertyChangedEventArgs e)
{
    Console.Clear();

    Console.WriteLine($"           |    Latitude |    Longitude |  Speed | Heading");
    Console.WriteLine($"{leftGpsState.Title}   | {leftGpsState.Latitude:000.0000000} | {leftGpsState.Longitude:000.0000000} | {leftGpsState.Speed:000.00} |  {leftGpsState.Heading:000.00}          ");
    Console.WriteLine($"{rightGpsState.Title}  | {rightGpsState.Latitude:000.0000000} | {rightGpsState.Longitude:000.0000000} | {rightGpsState.Speed:000.00} |  {rightGpsState.Heading:000.00}          ");
    Console.WriteLine($"{gpsMixerState.Title}  | {gpsMixerState.Latitude:000.0000000} | {gpsMixerState.Longitude:000.0000000} | {gpsMixerState.Speed:000.00} |  {gpsMixerState.Heading:000.00}          ");
    Console.WriteLine($"Distance: {gpsMixerState.Distance}/{gpsMixerState.Distance * 1000}");

    SendToHub("UpdateGpsMixerState", gpsMixerState);
}

void SendToHub(string methodName, object arg1)
{
    if (hubConnection.State == HubConnectionState.Connected)
        hubConnection.InvokeAsync(methodName, arg1);
}
