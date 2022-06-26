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
    public class RoboteqConnection : Item, IConnection
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
                //Send("^ECHOF 1");
                //Send("?A");                         // Motor amps by channel
                //Send("?A");                         // repeating first one, sometimes first query doesn't work
                //Send("?BA");                        // Battery amps by channel
                //Send("?M");                         // Motor Command Applied
                //Send("?MXMD");                      // Mixing
                //Send("?T");                         // Temperature
                //Send("?V");                         // Volts
                //Send(string.Format("# {0}", 200));  // polling interval milliseconds
                //Send("^MXMD 0");                    // Turn off mixing
                _IsOpen = true;
                CloseCommand.CanExecute = true;
                OpenCommand.CanExecute = false;
            }
        }
        public Command OpenCommand { get; private set; }

        public ObservableCollection<string> Output { get; private set; }

        public RoboteqConnection(string portName) 
        {
            _CommandLine = "";
            _IsOpen = false;

            _PortName = portName;
            _RoboteqPort = new SerialPort
            {
                BaudRate = 115200,
                DataBits = 8,
                DiscardNull = false,
                DtrEnable = true,
                Handshake = Handshake.None,
                NewLine = "\r",
                Parity = Parity.None,
                PortName = _PortName,
                ReadTimeout = 200,
                RtsEnable = true,
                StopBits = StopBits.One,
                WriteTimeout = 200
            };

            //_RoboteqPort = new SerialPort
            //{
            //    BaudRate = 9600,
            //    DataBits = 7,
            //    DiscardNull = false,
            //    DtrEnable = true,
            //    Handshake = Handshake.None,
            //    NewLine = "\r",
            //    Parity = Parity.Even,
            //    PortName = _PortName,
            //    ReadTimeout = 200,
            //    RtsEnable = true,
            //    StopBits = StopBits.One,
            //    WriteTimeout = 200
            //};
            _RoboteqPort.DataReceived += _RoboteqPort_DataReceived;

            CloseCommand = new Command(Close, false);
            ExecuteCommandCommand = new Command(ExecuteCommand, true);
            OpenCommand = new Command(Open, true);
            Output = new ObservableCollection<string>();
        }

        private void _RoboteqPort_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            var newLine = _RoboteqPort.ReadLine();
            _RoboteqPort.DiscardInBuffer();
            Output.Add(newLine);
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

        public string SensorPath { get => throw new NotImplementedException(); set => throw new NotImplementedException(); }

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
