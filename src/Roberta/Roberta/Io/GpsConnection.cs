using System.IO.Ports;
using System.Runtime.Serialization;

namespace Roberta.Io
{
    public class GpsConnection : IConnection
    {
        private readonly GpsState _GpsState;
        readonly SerialPort _SerialPort;

        public void Close()
        {
            this._IsOpen = _GpsState.IsReady = false;
            _SerialPort.DataReceived -= SerialPort_DataReceived;
            if (_SerialPort.IsOpen)
                _SerialPort.Close();
        }

        private readonly string _ConnectionString;
        public string ConnectionString { get { return _ConnectionString; } }

        public void Dispose()
        {
            Close();
            _SerialPort?.Dispose();
        }

        static public double GetDegrees(string degreesMinutes, string hemishpere)
        {
            // NMEA data is in the format of "[D]DDMM.MMMM,H" where:
            // Lat uses two Ds, Lon uses three Ds
            // Hemishpere is N, S, E or W, where numerical representation has S and W as negatvie numbers

            double returnValue = 0D;

            if (string.IsNullOrWhiteSpace(degreesMinutes) || string.IsNullOrWhiteSpace(hemishpere))
                return returnValue;

            var dotIndex = degreesMinutes.IndexOf(".");

            var degrees = double.Parse(degreesMinutes[..(dotIndex - 2)]);
            var minutes = double.Parse(degreesMinutes[(dotIndex - 2)..]);
            returnValue = degrees + (minutes / 60);

            if (hemishpere == "S" || hemishpere == "W")
                returnValue = -returnValue;

            return returnValue;
        }

        public GpsConnection(string connectionString, GpsState gpsState)
        {
            _GpsState = gpsState;
            _ConnectionString = connectionString;
            _SerialPort = new SerialPort
            {
                PortName = connectionString,
                BaudRate = 4800,
                Parity = Parity.None,
                DataBits = 8,
                StopBits = StopBits.One
            };
        }

        private bool _IsOpen;
        public bool IsOpen
        {
            get { return _IsOpen; }
        }

        public void Open()
        {
            if (!_IsOpen)
                Task.Run(ReadData);

            //// Switch from SiRF Binary to NMEA Output
            //string str = "A0 A2 00 18 81 02 01 01 00 01 01 01 05 01 01 01 00 01 00 01 00 00 00 01 00 00 12 C0 01 65 B0 B3";
            //byte[] bytes = str.Split(' ').Select(s => Convert.ToByte(s, 16)).ToArray();
            //_SerialPort.Write(bytes, 0, bytes.Length);
        }

        private void ReadData()
        {
            _IsOpen = _GpsState.IsReady = false;
            while (_IsOpen == false)
            {
                try
                {
                    _SerialPort.Open();
                    _SerialPort.DataReceived += SerialPort_DataReceived;
                    _IsOpen = _GpsState.IsReady = true;
                }
                catch (Exception ex)
                {
                    Console.WriteLine(ex);
                    Thread.Sleep(5000);
                }
            }
        }

        private void SerialPort_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            string newLine;

            try
            {
                if (!_SerialPort.IsOpen) return;
                newLine = _SerialPort.ReadLine();
                _SerialPort.DiscardInBuffer();
            }
            catch
            {
                return;
            }

            if (string.IsNullOrWhiteSpace(newLine)) return;
            bool isValid = false;
            string sentenceToCheck = "";

            try
            {
                sentenceToCheck = newLine.Trim();
                int checksum = Convert.ToByte(sentenceToCheck[sentenceToCheck.IndexOf('$') + 1]);
                for (int i = sentenceToCheck.IndexOf('$') + 2; i < sentenceToCheck.IndexOf('*'); i++)
                    checksum ^= Convert.ToByte(sentenceToCheck[i]);
                var sentenceChecksum = sentenceToCheck.Split(new char[] { '*' }, StringSplitOptions.RemoveEmptyEntries);
                isValid = sentenceChecksum[1] == checksum.ToString("X2");
            }
            catch { }

            if (!isValid) return;

            //Console.WriteLine(sentenceToCheck);

            var fields = sentenceToCheck.Split(new char[] { ',' });

            switch (fields[0])
            {
                case "$GPRMC":
                    double lat, lon, speed, heading;
                    lat = GetDegrees(fields[3], fields[4]);
                    lon = GetDegrees(fields[5], fields[6]);
                    double.TryParse(fields[7], out speed);
                    double.TryParse(fields[8], out heading);
                    _GpsState.SetVals(lat, lon, speed, heading);
                    //Console.WriteLine($"{lat}, {lon}, {speed}, {heading}");
                    break;
            }
        }
    }
}
