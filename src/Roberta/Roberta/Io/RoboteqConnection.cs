using Roberta.ComponentModel;
using Roberta.Win.Input;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO.Ports;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;

namespace Roberta.Io
{
    public class RoboteqConnection : ItemState, IDisposable
    {
        private bool _IsOpen;
 
        private readonly SerialPort _RoboteqPort;
        public  SerialPort RoboteqPort { get { return _RoboteqPort;} }

        public virtual void Close()
        {
            if (_IsOpen)
            {
                _RoboteqPort.Close();
                _IsOpen = false;
                CloseCommand.CanExecute = false;
                OpenCommand.CanExecute = true;
            }
        }
        public Command CloseCommand { get; private set; }

        private string _CommandLine;
        [DataMember]
        public virtual string CommandLine
        {
            get { return this._CommandLine; }
            set
            {
                if (value != this._CommandLine)
                {
                    this._CommandLine = value;
                    this.RaisePropertyChanged(nameof(CommandLine));
                }
            }
        }

        public void Dispose()
        {
            if (null != _RoboteqPort)
            {
                if (_RoboteqPort.IsOpen)
                    _RoboteqPort.Close();
                _RoboteqPort.Dispose();
            }
        }

        public virtual void ExecuteCommand()
        {
            Send(_CommandLine);
        }
        public Command ExecuteCommandCommand { get; private set; }

        public virtual void Open()
        {
            if (!_IsOpen)
            {
                _RoboteqPort.Open();
                Send("^ECHOF 1");
                Send("~MXMD");                      // Query Mix Mode
                Send("?A");                         // Motor amps by channel
                Send("?A");                         // repeating first one, sometimes first query doesn't work
                Send("?BA");                        // Battery amps by channel
                Send("?M");                         // Motor Command Applied
                Send("?T");                         // Temperature
                Send("?V");                         // Volts
                Send("^MXMD 0");                    // Turn off mixing
                Send(string.Format("# {0}", 200));  // polling interval milliseconds
                Send("~MXMD");                      // Query Mix Mode
                _IsOpen = true;
                CloseCommand.CanExecute = true;
                OpenCommand.CanExecute = false;
            }
        }
        public Command OpenCommand { get; private set; }

        public RoboteqConnection(string portName, RoboteqState roboteqState) 
        {
            _CommandLine = "";
            _IsOpen = false;

            _PortName = portName;
            _RoboteqState = roboteqState;

            _RoboteqPort = new SerialPort
            {
                BaudRate = 9600,
                DataBits = 7,
                DiscardNull = false,
                DtrEnable = true,
                Handshake = Handshake.None,
                NewLine = "\r",
                Parity = Parity.Even,
                PortName = _PortName,
                ReadTimeout = 200,
                RtsEnable = true,
                StopBits = StopBits.One,
                WriteTimeout = 200
            };
            _RoboteqPort.DataReceived += _RoboteqPort_DataReceived;

            CloseCommand = new Command(Close, false);
            ExecuteCommandCommand = new Command(ExecuteCommand, true);
            OpenCommand = new Command(Open, true);
        }

        private void _RoboteqPort_DataReceived(object sender, SerialDataReceivedEventArgs e)
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

        private string _PortName;
        [DataMember]
        public virtual string PortName
        {
            get { return this._PortName; }
            set
            {
                if (value != this._PortName)
                {
                    this._PortName = value;
                    this.RaisePropertyChanged(nameof(PortName));
                }
            }
        }

        private RoboteqState _RoboteqState;
        [DataMember]
        public virtual RoboteqState RoboteqState
        {
            get { return this._RoboteqState; }
            set
            {
                if (value != this._RoboteqState)
                {
                    this._RoboteqState = value;
                    this.RaisePropertyChanged(nameof(RoboteqState));
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
