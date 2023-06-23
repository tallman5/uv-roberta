using Roberta.Io;
using System.Collections.Generic;

namespace Roberta.Cli
{
    internal class Program
    {
        static void Main(string[] args)
        {
            Console.Clear();

            var gamepadPath = "/dev/input/by-id/usb-FrSky_FrSky_Simulator_4995316A3546-joystick";
            //var gpsPath = "/dev/serial/by-id/usb-Prolific_Technology_Inc._USB-Serial_Controller_D-if00-port0";
            //var roboteqPath = "/dev/serial/by-id/usb-Roboteq_Motor_Controller_MDC2XXX-if00";

            //#if DEBUG
            //            roboteqPath = "COM4";
            //            gpsPath = "COM5";
            //#endif

            var connections = new List<IConnection>();

            //var gpsState = new GpsState();
            //var gpsConnection = new GpsConnection(gpsPath, gpsState);
            //connections.Add(gpsConnection);

            //var roboteqState = new RoboteqState();
            //var roboteqConnection = new RoboteqConnection(roboteqPath, roboteqState);
            //connections.Add(roboteqConnection);

            var rxState = new RxState();
            rxState.PropertyChanged += RxState_PropertyChanged;
            var rxConnection = new RxConnection(gamepadPath, rxState);
            connections.Add(rxConnection);
            Console.WriteLine("Opening Gamepad...");
            rxConnection.Open();

            Console.WriteLine("Running, press enter to exit.");
            Console.ReadLine();

            Console.WriteLine("Closing connections...");
            foreach (var connection in connections)
            {
                connection.Close();
                connection.Dispose();
            }
        }

        private static void RxState_PropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            if (sender is not RxState state) return;
            Console.SetCursorPosition(0, 2);
            Console.WriteLine($"Failsafe:  {state.InFailsafe}              ");
            Console.WriteLine($"Armed:     {state.IsArmed}                 ");
            for (var i = 0; i < state.ChannelValues.Count; i++)
                Console.WriteLine($"Channel {i + 1}: {state.ChannelValues[i]}                 ");
        }
    }
}