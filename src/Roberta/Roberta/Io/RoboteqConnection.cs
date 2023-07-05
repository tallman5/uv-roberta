using System.IO.Ports;

namespace Roberta.Io
{
    public class RoboteqConnection : IConnection
    {
        private readonly SerialPort _RoboteqPort;
        private readonly RoboteqState _RoboteqState;

        public virtual void Close()
        {
            _IsOpen = false;
            _RoboteqState.IsReady = false;
            _RoboteqPort.DataReceived -= RoboteqPort_DataReceived;
            if (_RoboteqPort.IsOpen)
                _RoboteqPort.Close();
        }

        private readonly string _ConnectionString;
        public string ConnectionString { get { return _ConnectionString; } }

        public void Dispose()
        {
            Close();
            _RoboteqPort?.Dispose();
        }

        private bool _IsOpen;
        public bool IsOpen
        {
            get { return _IsOpen; }
        }

        public virtual void Open()
        {
            if (!_IsOpen)
                Task.Run(ReadData);
        }

        private void ReadData()
        {
            _IsOpen = _RoboteqState.IsReady = false;
            while (_IsOpen == false)
            {
                try
                {
                    _RoboteqPort.Open();
                    Send("^ECHOF 1");                   // Echo off
                    //Send("~MXMD");                      // Query Mix Mode
                    Send("^MXMD 1");
                    //Send("?A");                         // Motor amps by channel
                    //Send("?A");                         // repeating first one, sometimes first query doesn't work
                    //Send("?BA");                        // Battery amps by channel
                    //Send("?M");                         // Motor Command Applied
                    //Send("?T");                         // Temperature
                    //Send("?V");                         // Volts
                    //Send("^MXMD 0");                    // Turn off mixing
                    //Send(string.Format("# {0}", 200));  // polling interval milliseconds
                    //Send("~MXMD");                      // Query Mix Mode

                    //_RoboteqPort.DataReceived += RoboteqPort_DataReceived;
                    Send("^MXMD 1");
                    _IsOpen = _RoboteqState.IsReady = true;
                }
                catch
                {
                    Thread.Sleep(5000);
                }
            }
        }

        public RoboteqConnection(string connectionString, RoboteqState roboteqState)
        {
            _IsOpen = false;
            _ConnectionString = connectionString;
            _RoboteqPort = new SerialPort
            {
                BaudRate = 9600,
                DataBits = 7,
                DiscardNull = false,
                DtrEnable = true,
                Handshake = Handshake.None,
                NewLine = "\r",
                Parity = Parity.Even,
                PortName = _ConnectionString,
                ReadTimeout = 200,
                RtsEnable = true,
                StopBits = StopBits.One,
                WriteTimeout = 200
            };
            _RoboteqState = roboteqState;
        }

        private void RoboteqPort_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            string newLine;
            try
            {
                newLine = _RoboteqPort.ReadLine();
            }
            catch { return; }

            if (string.IsNullOrWhiteSpace(newLine) || newLine == "+") return;

            decimal tempDecimal;

            if (newLine.StartsWith("A="))
            {
                var values = newLine.Replace("A=", "").Split(':');
                if (decimal.TryParse(values[0], out tempDecimal))
                    _RoboteqState.LeftMotorState.MotorAmps = tempDecimal / 10;
                if (decimal.TryParse(values[1], out tempDecimal))
                    _RoboteqState.RightMotorState.MotorAmps = tempDecimal / 10;
            }

            if (newLine.StartsWith("BA="))
            {
                var values = newLine.Replace("BA=", "").Split(':');
                if (decimal.TryParse(values[0], out tempDecimal))
                    _RoboteqState.LeftMotorState.BatteryAmps = tempDecimal / 10;
                if (decimal.TryParse(values[1], out tempDecimal))
                    _RoboteqState.RightMotorState.BatteryAmps = tempDecimal / 10;
            }

            if (newLine.StartsWith("M="))
            {
                var values = newLine.Replace("M=", "").Split(':');
                if (decimal.TryParse(values[0], out tempDecimal))
                    _RoboteqState.RightMotorState.Power = tempDecimal / 10;
                if (decimal.TryParse(values[1], out tempDecimal))
                    _RoboteqState.LeftMotorState.Power = tempDecimal / 10;
            }

            if (newLine.StartsWith("MXMD="))
            {
                var values = newLine.Replace("MXMD=", "").Split(':');
                if (values[0] == "0")
                    _RoboteqState.MixMode = MixMode.Separate;
                if (values[0] == "1")
                    _RoboteqState.MixMode = MixMode.Mode1;
                if (values[0] == "2")
                    _RoboteqState.MixMode = MixMode.Mode2;
            }

            if (newLine.StartsWith("T="))
            {
                var values = newLine.Replace("T=", "").Split(':');

                if (decimal.TryParse(values[0], out tempDecimal))
                    _RoboteqState.MainTemperature = tempDecimal;
                if (decimal.TryParse(values[1], out tempDecimal))
                    _RoboteqState.LeftMotorState.Temperature = tempDecimal;
                if (decimal.TryParse(values[2], out tempDecimal))
                    _RoboteqState.RightMotorState.Temperature = tempDecimal;
            }

            if (newLine.StartsWith("V="))
            {
                var values = newLine.Replace("V=", "").Split(':');
                if (decimal.TryParse(values[1], out tempDecimal))
                {
                    var battVolts = tempDecimal / 10;
                    _RoboteqState.BatteryState.Volts = battVolts;
                    _RoboteqState.BatteryState.Percent = (int)(((battVolts - _RoboteqState.BatteryState.MinVoltage) / (_RoboteqState.BatteryState.MaxVoltage - _RoboteqState.BatteryState.MinVoltage)) * 100);
                }
            }
        }

        public void Send(string line)
        {
            if (null == _RoboteqPort || !_RoboteqPort.IsOpen) return;
            _RoboteqPort.DiscardOutBuffer(); // clear previous lines which may not have been processed
            _RoboteqPort.WriteLine(line);
        }

        // Priority
        // Syntax: ^CPRI pp nn
        // pp: = priority
        // nn = command, 0 - Serial, 1 for RC

        // Mix Mode
        // Syntax: ^MXMD nn
        // nn = mode, 0: Separate, 1: Mode 1, 2: Mode 2
    }
}
