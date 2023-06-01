using Roberta.Io;
using System.IO.Ports;

namespace Roberta.Cli
{
    public class Program
    {
        static bool keepRunning;
        static GpsState? gpsState;
        static GpsConnection? gpsConnection;
        static RoboteqState? roboteqState;
        static RoboteqConnection? roboteqConnection;

        static void Main(string[] args)
        {
            var gpsPortName = "/dev/serial/by-id/usb-Prolific_Technology_Inc._USB-Serial_Controller_D-if00-port0";
            var roboteqPortName = "/dev/serial/by-id/usb-Roboteq_Motor_Controller_MDC2XXX-if00";

#if DEBUG
            roboteqPortName = "COM4";
            gpsPortName = "COM5";
#endif
            var connectGps = false;
            var connectRoboteq = false;
            if (args.Length > 0)
            {
                foreach (var arg in args)
                {
                    if (arg.ToLower() == "-g") connectGps = true;
                    if (arg.ToLower() == "-r") connectRoboteq = true;
                }
            }
            else
            {
                connectGps = true;
                connectRoboteq = true;
            }

            Console.Clear();
            WriteTable();

            gpsState = new GpsState();
            gpsState.PropertyChanged += GpsState_PropertyChanged;
            gpsConnection = new GpsConnection(gpsPortName, gpsState);
            if (connectGps)
                gpsConnection.Open();

            roboteqState = new RoboteqState();
            roboteqState.PropertyChanged += RoboteqState_PropertyChanged;
            roboteqConnection = new RoboteqConnection(roboteqPortName, roboteqState);
            if (connectRoboteq)
                roboteqConnection.Open();

            keepRunning = true;
            Console.CancelKeyPress += Console_CancelKeyPress;
            while (keepRunning)
                Thread.Sleep(1000);

            gpsConnection?.Dispose();
            roboteqConnection?.Dispose();
        }

        private static void Console_CancelKeyPress(object? sender, ConsoleCancelEventArgs e)
        {
            MoveCursorToEnd();
            Console.WriteLine("Cleaning up resources...");
            keepRunning = false;
            gpsConnection?.Dispose();
            roboteqConnection?.Dispose();
        }

        private static void GpsState_PropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            if (!keepRunning) return;
            var state = sender as GpsState;
            if (state is null) return;

            Console.SetCursorPosition(32, 3);
            Console.Write(state.Latitude.ToString("00.#####"));

            Console.SetCursorPosition(32, 4);
            Console.Write(state.Longitude.ToString("00.#####"));

            Console.SetCursorPosition(32, 5);
            Console.Write(state.Heading.ToString("00.#####"));

            Console.SetCursorPosition(32, 6);
            Console.Write(state.Speed.ToString("00.#####"));

            MoveCursorToEnd();
        }

        private static void MoveCursorToEnd()
        {
            Console.CursorTop = 12;
            Console.CursorLeft = 0;
        }

        private static void RoboteqState_PropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            if (!keepRunning) return;
            var state = sender as RoboteqState;
            if (state is null) return;

            Console.SetCursorPosition(57, 3);
            Console.Write("{0}", state.BatteryState.Volts);

            Console.CursorLeft = 57;
            Console.SetCursorPosition(57, 4);
            Console.Write("{0}   ", state.BatteryState.Percent);

            Console.CursorLeft = 57;
            Console.SetCursorPosition(57, 5);
            Console.Write("{0}", state.CommandPriority);

            //Console.CursorLeft = 57;
            //Console.SetCursorPosition(57, 6);
            //Console.Write("{0}    ", roboteqState.PowerScale);

            Console.CursorLeft = 57;
            Console.SetCursorPosition(57, 7);
            Console.Write("{0}/{1}     ", state.LeftMotorState.BatteryAmps, state.RightMotorState.BatteryAmps);

            Console.CursorLeft = 57;
            Console.SetCursorPosition(57, 8);
            Console.Write("{0}/{1}     ", state.LeftMotorState.MotorAmps, state.RightMotorState.MotorAmps);

            Console.SetCursorPosition(57, 9);
            Console.Write(String.Format("{0}", state.MixMode).PadRight(10));

            MoveCursorToEnd();
        }

        private static void WriteTable()
        {
            Console.WriteLine("========================================================================");
            Console.WriteLine("|    Hub         |        GPS             |    Roboteq                 |");
            Console.WriteLine("------------------------------------------------------------------------");
            Console.WriteLine("|  Disconnected  |   Latitude:            |      Volts:  00            |");
            Console.WriteLine("|                |  Longitude:            |  Battery %:  00            |");
            Console.WriteLine("|                |    Heading:            |   Priority:  Transmitter   |");
            Console.WriteLine("|                |      Speed:            |      Scale:  000           |");
            Console.WriteLine("|                |                        |     B Amps:  000           |");
            Console.WriteLine("|                |                        |     M Amps:  000           |");
            Console.WriteLine("|                |                        |   Mix Mode:  Separate      |");
            Console.WriteLine("========================================================================");
        }
    }
}
