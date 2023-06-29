using Microsoft.AspNetCore.SignalR.Client;
using Roberta.Io;
using System.ComponentModel;

namespace Roberta.Cli
{
    internal class Program
    {
        private static readonly object lockObject = new();
        private static RoboteqConnection? roboteqConnection;

        static void Main(string[] args)
        {
            Console.Clear();

            //Defaults for Raspberry Pi
            var baseUrl = "https://192.168.1.156:7142";
            var gamepadPath = "/dev/input/by-id/usb-FrSky_FrSky_Simulator_4995316A3546-joystick";
            var gpsPath = "/dev/serial/by-id/usb-Prolific_Technology_Inc._USB-Serial_Controller_D-if00-port0";
            var roboteqPath = "/dev/serial/by-id/usb-Roboteq_Motor_Controller_MDC2XXX-if00";

#if DEBUG
            baseUrl = "https://192.168.1.156:7142";
            gpsPath = "COM5";
            roboteqPath = "COM4";
#endif

            var connections = new List<IConnection>();

            var gpsState = new GpsState { Title = "Left GPS" };
            var gpsConnection = new GpsConnection(gpsPath, gpsState);
            connections.Add(gpsConnection);

            var roboteqState = new RoboteqState { Title = "Roboteq" };
            roboteqConnection = new RoboteqConnection(roboteqPath, roboteqState);
            connections.Add(roboteqConnection);

            var rxState = new RxState { Title = "RX Dongle" };
            var rxConnection = new RxConnection(gamepadPath, rxState);
            connections.Add(rxConnection);

            foreach (var connection in connections)
            {
                Console.WriteLine($"Opening {connection.ConnectionString} connection...");
                connection.Open();
            }

            var handler = new HttpClientHandler
            {
                // Ignore certificate validation errors
                ServerCertificateCustomValidationCallback = (sender, cert, chain, sslPolicyErrors) => true
            };
            Console.WriteLine("Connecting to hub...");
            var hubUrl = baseUrl + "/robertaHub";
            HubConnection hubConnection = new HubConnectionBuilder()
                .WithUrl(hubUrl, options =>
                {
                    options.HttpMessageHandlerFactory = _ => handler;
                })
                .WithAutomaticReconnect()
                .Build();
            hubConnection.StartAsync();

            Console.WriteLine();
            Console.WriteLine("Running, press enter to exit.");

            var pcea = new PropertyChangedEventArgs("IsReady");
            gpsState.PropertyChanged += GpsState_PropertyChanged;
            GpsState_PropertyChanged(gpsState, pcea);
            roboteqState.PropertyChanged += RoboteqState_PropertyChanged;
            RoboteqState_PropertyChanged(roboteqState, pcea);
            rxState.PropertyChanged += RxState_PropertyChanged;
            RxState_PropertyChanged(rxState, pcea);

            Console.ReadLine();

            MoveToEnd();

            foreach (var connection in connections)
            {
                Console.WriteLine($"Closing {connection.ConnectionString}...");
                connection.Dispose();
            }
        }

        private static void GpsState_PropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            lock (lockObject)
            {
                if (sender is not GpsState state) return;
                Console.SetCursorPosition(0, 7);
                Console.WriteLine($"{state.Title}, Is Ready: {state.IsReady}          ");
                Console.WriteLine($"  Latitude |  Longitude |  Speed | Heading");
                Console.WriteLine($" {state.Latitude:000.00000} | {state.Longitude:000.00000} | {state.Speed:000.00} |  {state.Heading:000.00}          ");
                MoveToEnd();
            }
        }

        private static void MoveToEnd()
        {
            Console.SetCursorPosition(0, 27);
        }

        private static void RoboteqState_PropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            lock (lockObject)
            {
                MoveToEnd();
            }
        }

        private static void RxState_PropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            if (sender is not RxState state) return;

            lock (lockObject)
            {
                Console.SetCursorPosition(0, 11);
                Console.WriteLine($"{state.Title}, Is Ready: {state.IsReady}          ");
                Console.WriteLine($"Armed:      {state.IsArmed}          ");
                Console.WriteLine($"Pilot Mode: {state.PilotMode}          ");
                for (var i = 0; i < state.ChannelValues.Count; i++)
                    Console.WriteLine($"Channel {i + 1}: {state.ChannelValues[i]}          ");
                MoveToEnd();
            }

            if (state.IsArmed && state.IsReady && state.PilotMode == PilotMode.Transmitter
                && null != roboteqConnection && roboteqConnection.IsOpen)
            {
                var l = Utilities.ScaleValue(state.Channel03);
                var r = Utilities.ScaleValue(state.Channel04);
                var newLine = $"!M {l} {r}";
                roboteqConnection.Send(newLine);
            }
        }
    }
}