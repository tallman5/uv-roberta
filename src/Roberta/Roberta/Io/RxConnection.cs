using Roberta.Win.Input;
using System.Runtime.Serialization;

namespace Roberta.Io
{
    public class RxConnection : IConnection
    {
        private RxState _RxState;
        private FileStream? _Stream;

        public virtual void Close()
        {
            if (_IsOpen)
            {
                _IsOpen = false;
                CloseCommand.CanExecute = false;
                OpenCommand.CanExecute = true;
                _Stream?.Close();
            }
        }
        public Command CloseCommand { get; private set; }

        public void Dispose()
        {
            _Stream?.Dispose();
        }

        private bool _IsOpen;
        public bool IsOpen
        {
            get { return _IsOpen; }
        }

        public virtual void Open()
        {
            if (!_IsOpen)
            {
                _IsOpen = true;
                CloseCommand.CanExecute = true;
                OpenCommand.CanExecute = false;
                Task.Run(ReadData);
            }
        }
        public Command OpenCommand { get; private set; }

        private string _PortName;
        [DataMember]
        public string PortName
        {
            get { return _PortName; }
        }

        private void ReadData()
        {
            try
            {
                _Stream = new FileStream(_PortName, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);

                byte[] buffer = new byte[8];

                while (_IsOpen == true)
                {
                    _Stream.Read(buffer, 0, buffer.Length);
                    var value = ScaleValue((BitConverter.ToInt16(buffer, 4)));
                    var number = buffer[7];
                    if (number < 7)
                        this._RxState.ChannelValues[number] = value;

                    Thread.Sleep(10);
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex);
            }
        }

        public RxConnection(string portName, RxState rxState)
        {
            _IsOpen = false;
            _PortName = portName;
            _RxState = rxState;
            CloseCommand = new Command(Close, false);
            OpenCommand = new Command(Open, true);
        }

        public int ScaleValue(int value)
        {
            return ScaleValue(value, -32768, 32768, 1000, 2000);
        }

        public int ScaleValue(int value, int minValue, int maxValue, int scaledMinValue, int scaledMaxValue)
        {
            int scaledValue = scaledMinValue + (int)((double)(value - minValue) / (maxValue - minValue) * (scaledMaxValue - scaledMinValue));
            return scaledValue;
        }
    }
}
