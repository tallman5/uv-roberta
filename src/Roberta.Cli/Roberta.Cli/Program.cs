using Roberta.Io;
using System.IO.Ports;

var gpsSensorPath = "/dev/ttyUSB0";

#if DEBUG
roboteqPortName = "COM4";
gpsSensorPath = "COM5";
#endif

try
{
    var gpsPort = new SerialPort
    {
        PortName = gpsSensorPath,
        BaudRate = 4800,
        Parity = Parity.None,
        DataBits = 8,
        StopBits = StopBits.One
    };
    gpsPort.Open();
    while (true)
    {
        var line = gpsPort.ReadLine();
        if (line != null)
        {
            Console.WriteLine(line);
        }
    }

}
catch (Exception ex)
{
    Console.WriteLine(ex.Message);
}


//var roboteqPortName = "/dev/serial/by-id/usb-Roboteq_Motor_Controller_MDC2XXX-if00";
//Console.WriteLine("Connecting to Roboteq on {0}...", roboteqPortName);
//using var roboteqConnection = new RoboteqConnection(roboteqPortName);
//roboteqConnection.Open();

//foreach (var arg in args)
//{
//    Console.WriteLine("Executing {0}...", arg);
//    roboteqConnection.Send(arg);
//    Thread.Sleep(100);
//}

//for (int i = 0; i < 100; i++)
//{
//    var log = roboteqConnection.Output.ToList();
//    roboteqConnection.Output.Clear();
//    foreach (var line in log)
//    {
//        Console.WriteLine(line);
//    }
//    Thread.Sleep(100);
//}

//Console.WriteLine("Closing connection...");
//roboteqConnection.Close();

//Console.WriteLine("Log:");
//foreach (var line in roboteqConnection.Output)
//    Console.WriteLine(line);