namespace Roberta.Io
{
    public class RxConnection : IConnection
    {
        private readonly RxState _RxState;
        private FileStream? _Stream;

        public virtual void Close()
        {
            _IsOpen = false;
            _RxState.IsReady = false;
            _Stream?.Close();
        }

        private readonly string _ConnectionString;
        public string ConnectionString { get { return _ConnectionString; } }

        public void Dispose()
        {
            Close();
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
                Task.Run(ReadData);
        }

        private void ReadData()
        {
            _IsOpen = _RxState.IsReady = false;
            while (_IsOpen == false)
            {
                try
                {
                    _Stream = new FileStream(_ConnectionString, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
                    _IsOpen = true;
                    _RxState.IsReady = true;
                }
                catch
                {
                    Thread.Sleep(5000);
                }
            }

            byte[] buffer = new byte[8];

            try
            {
                while (_IsOpen == true)
                {
                    _Stream?.Read(buffer, 0, buffer.Length);
                    var value = BitConverter.ToInt16(buffer, 4);
                    var number = buffer[7];
                    if (number < 7)
                        this._RxState.ChannelValues[number] = value;
                    Thread.Sleep(1);
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex);
            }
        }

        public RxConnection(string connectionString, RxState rxState)
        {
            _IsOpen = false;
            _ConnectionString = connectionString;
            _RxState = rxState;
        }
    }
}
