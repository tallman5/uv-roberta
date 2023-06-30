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

        static public double GetDegrees(string DM, string Dir)
        {
            try
            {
                double returnValue = 0D;

                if (string.IsNullOrWhiteSpace(DM) || string.IsNullOrWhiteSpace(Dir))
                    return returnValue;

                string t = DM[DM.IndexOf(".")..];
                double FM = double.Parse(DM[DM.IndexOf(".")..]);

                //Get the minutes.
                t = DM.Substring(DM.IndexOf(".") - 2, 2);
                double Min = double.Parse(DM.Substring(DM.IndexOf(".") - 2, 2));

                //Degrees
                t = DM.Substring(0, DM.IndexOf(".") - 2);
                returnValue = double.Parse(DM[..(DM.IndexOf(".") - 2)]);

                if (Dir == "S" || Dir == "W")
                    returnValue = -(returnValue + (Min + FM) / 60);
                else
                    returnValue += (Min + FM) / 60;

                return returnValue;
            }
            catch (Exception ex)
            {
                var message = string.Format("Error getting degrees from DM/Dir of {0}/{1}", DM, Dir);
                throw new Exception(message, ex);
            }
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
                catch(Exception ex)
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

            var fields = sentenceToCheck.Split(new char[] { ',' });

            switch (fields[0])
            {
                case "$GPGGA":
                    _GpsState.Latitude = GetDegrees(fields[2], fields[3]);
                    _GpsState.Longitude = GetDegrees(fields[4], fields[5]);
                    break;
                case "$GPGSA":
                    break;
                case "$GPGSV":
                    break;
                case "$GPRMC":
                    double speed;
                    if (double.TryParse(fields[7], out speed))
                        _GpsState.Speed = speed;
                    else
                        _GpsState.Speed = 0;
                    double heading;
                    if (double.TryParse(fields[8], out heading))
                        _GpsState.Heading = heading;
                    else
                        _GpsState.Heading = 0;
                    break;
            }
        }
    }
}
