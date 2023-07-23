using Microsoft.AspNetCore.SignalR.Client;
using Roberta;
using Roberta.Io;
using System.ComponentModel;


Console.Clear();


var ipAddress = Utilities.GetLocalIPAddress();
var hubUrl = $"http://{ipAddress}:5000/robertaHub";
string leftConnection = "/dev/serial/by-id/usb-Prolific_Technology_Inc._USB-Serial_Controller_D-if00-port0";
var lastGpsTs = DateTimeOffset.MinValue;


#if DEBUG
leftConnection = "COM5";
#endif


// Hub Connection
HubConnection hubConnection;
var handler = new HttpClientHandler
{
    // TODO: Add cert to Raspberry Pi, ignoring cert errors for now
    ServerCertificateCustomValidationCallback = (sender, cert, chain, sslPolicyErrors) => true
};
Console.Write("Connecting to hub...");
hubConnection = new HubConnectionBuilder()
    .WithUrl(hubUrl, options =>
    {
        options.HttpMessageHandlerFactory = _ => handler;
    })
    .WithAutomaticReconnect()
    .Build();
await hubConnection.StartAsync();
Console.WriteLine("done.");


var connections = new List<Roberta.Io.IConnection>();
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


Console.WriteLine("Running, press Enter to exit");
Console.ReadLine();


Console.WriteLine("Cleaning up resources...");
await hubConnection.StopAsync();
await hubConnection.DisposeAsync();


void LeftGpsState_PropertyChanged(object? sender, PropertyChangedEventArgs e)
{
    if (leftGpsState.Timestamp > lastGpsTs)
    {
        SendToHub("UpdateGpsState", leftGpsState);
        lastGpsTs = leftGpsState.Timestamp;
    }
}


void SendToHub(string methodName, object arg1)
{
    if (hubConnection.State == HubConnectionState.Connected)
        hubConnection.InvokeAsync(methodName, arg1);
}
