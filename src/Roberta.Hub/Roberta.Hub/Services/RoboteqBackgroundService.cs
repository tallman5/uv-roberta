using Microsoft.AspNetCore.SignalR;
using Roberta.Hub.Hubs;
using Roberta.Io;
using System.IO.Ports;

namespace Roberta.Hub.Services
{
    public class RoboteqBackgroundService : BaseBackgroundService
    {
        private readonly SerialPort _SerialPort;
        private RoboteqState _state;
        private readonly string _portName;

        public RoboteqBackgroundService(IHubContext<RobertaHub> hubContext, ILogger<GpsBackgroundService> logger, IConfiguration configuration)
            : base(hubContext, logger, configuration)
        {
            _portName = _configuration["Roberta:RoboteqPort"];
            _state = new RoboteqState { Title = "Roboteq" };
            _SerialPort = new SerialPort
            {
                BaudRate = 9600,
                DataBits = 7,
                DiscardNull = false,
                DtrEnable = true,
                Handshake = Handshake.None,
                NewLine = "\r",
                Parity = Parity.Even,
                PortName = _portName,
                ReadTimeout = 200,
                RtsEnable = true,
                StopBits = StopBits.One,
                WriteTimeout = 200
            };
            _SerialPort.DataReceived += SerialPort_DataReceived;
        }

        protected override async Task ExecuteAsync(CancellationToken stoppingToken)
        {
            while (!stoppingToken.IsCancellationRequested)
            {
                if (!_SerialPort.IsOpen)
                {
                    try
                    {
                        _SerialPort.Open();
                        Send("^ECHOF 1");                   // Echo off
                        Send("^MXMD 0");                    // Set mix mode
                        Send("?A");                         // Motor amps by channel
                        Send("?A");                         // repeating first one, sometimes first query doesn't work
                        Send("?BA");                        // Battery amps by channel
                        Send("?M");                         // Motor Command Applied
                        Send("?T");                         // Temperature
                        Send("?V");                         // Volts
                        Send(string.Format("# {0}", 100));  // Polling interval milliseconds
                        Send("^MXMD 0");                    // Set mix mode
                        Send("^RWD 0");                     // Disable watchdog
                        Send("~MXMD");                      // Query Mix Mode
                        Console.WriteLine($"Connected to Roboteq on port {_portName}");
                    }
                    catch
                    {
                        Console.WriteLine($"Could not connect to Roboteq on port {_portName}, retrying in 10 seconds...");
                    }
                }
                await Task.Delay(TimeSpan.FromSeconds(10), stoppingToken);
            }

            if (null != _SerialPort)
            {
                if (_SerialPort.IsOpen) _SerialPort.Close();
                _SerialPort.Dispose();
            }
        }

        public void Send(string line)
        {
            if (null == _SerialPort || !_SerialPort.IsOpen) return;
            _SerialPort.DiscardOutBuffer(); // clear previous lines which may not have been processed
            _SerialPort.WriteLine(line);
        }

        private void SerialPort_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            string newLine;
            try
            {
                newLine = _SerialPort.ReadLine();
                _SerialPort.DiscardInBuffer();
                if (string.IsNullOrWhiteSpace(newLine) || newLine == "+") return;

                decimal tempDecimal;

                if (newLine.StartsWith("A="))
                {
                    var values = newLine.Replace("A=", "").Split(':');
                    if (decimal.TryParse(values[0], out tempDecimal))
                        _state.LeftMotorState.MotorAmps = tempDecimal / 10;
                    if (decimal.TryParse(values[1], out tempDecimal))
                        _state.RightMotorState.MotorAmps = tempDecimal / 10;
                }

                if (newLine.StartsWith("BA="))
                {
                    var values = newLine.Replace("BA=", "").Split(':');
                    if (decimal.TryParse(values[0], out tempDecimal))
                        _state.LeftMotorState.BatteryAmps = tempDecimal / 10;
                    if (decimal.TryParse(values[1], out tempDecimal))
                        _state.RightMotorState.BatteryAmps = tempDecimal / 10;
                }

                if (newLine.StartsWith("M="))
                {
                    var values = newLine.Replace("M=", "").Split(':');
                    if (decimal.TryParse(values[0], out tempDecimal))
                        _state.RightMotorState.Power = tempDecimal;
                    if (decimal.TryParse(values[1], out tempDecimal))
                        _state.LeftMotorState.Power = tempDecimal;
                }

                if (newLine.StartsWith("MXMD="))
                {
                    var values = newLine.Replace("MXMD=", "").Split(':');
                    if (values[0] == "0")
                        _state.MixMode = MixMode.Separate;
                    if (values[0] == "1")
                        _state.MixMode = MixMode.Mode1;
                    if (values[0] == "2")
                        _state.MixMode = MixMode.Mode2;
                }

                if (newLine.StartsWith("T="))
                {
                    var values = newLine.Replace("T=", "").Split(':');

                    if (decimal.TryParse(values[0], out tempDecimal))
                        _state.MainTemperature = tempDecimal;
                    if (decimal.TryParse(values[1], out tempDecimal))
                        _state.LeftMotorState.Temperature = tempDecimal;
                    if (decimal.TryParse(values[2], out tempDecimal))
                        _state.RightMotorState.Temperature = tempDecimal;
                }

                if (newLine.StartsWith("V="))
                {
                    var values = newLine.Replace("V=", "").Split(':');
                    if (decimal.TryParse(values[1], out tempDecimal))
                    {
                        var battVolts = tempDecimal / 10;
                        _state.BatteryState.Volts = battVolts;
                        _state.BatteryState.Percent = (int)(((battVolts - _state.BatteryState.MinVoltage) / (_state.BatteryState.MaxVoltage - _state.BatteryState.MinVoltage)) * 100);
                    }
                }

                _hubContext.Clients.All.SendAsync(RobertaHub.RoboteqStateUpdated, _state);
            }
            catch (Exception ex)
            {
                Console.WriteLine("An error occurred reading Roboteq Data:");
                Console.WriteLine(ex.Message);
            }
        }

        public void SetXY(decimal x, decimal y)
        {
            //var pm = PowerMixer.FromXY(x, y);
            //string newLine = $"!M {pm.Right}, {pm.Left}";
            //string newLine = $"!M {y}, {x}";
            string newLine = $"!M {y - x}, {x + y}";
            Send(newLine);
        }
    }
}
