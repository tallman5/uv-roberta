﻿using System.IO.Ports;

namespace Roberta.Io
{
    public class GpsConnection : IConnection
    {
        public void Close()
        {
            if (null != _SerialPort)
            {
                _SerialPort.DataReceived -= SerialPort_DataReceived;
                if (_SerialPort.IsOpen)
                    _SerialPort.Close();
            }
        }

        public void Dispose()
        {
            if (null != _SerialPort)
            {
                _SerialPort.DataReceived -= SerialPort_DataReceived;
                if (_SerialPort.IsOpen)
                    _SerialPort.Close();
                _SerialPort.Dispose();
            }
        }

        static public double GetDegrees(string DM, string Dir)
        {
            try
            {
                double returnValue = 0D;

                if (string.IsNullOrWhiteSpace(DM) || string.IsNullOrWhiteSpace(Dir))
                    return returnValue;

                string t = DM.Substring(DM.IndexOf("."));
                double FM = double.Parse(DM.Substring(DM.IndexOf(".")));

                //Get the minutes.
                t = DM.Substring(DM.IndexOf(".") - 2, 2);
                double Min = double.Parse(DM.Substring(DM.IndexOf(".") - 2, 2));

                //Degrees
                t = DM.Substring(0, DM.IndexOf(".") - 2);
                returnValue = double.Parse(DM.Substring(0, DM.IndexOf(".") - 2));

                if (Dir == "S" || Dir == "W")
                    returnValue = -(returnValue + (Min + FM) / 60);
                else
                    returnValue = returnValue + (Min + FM) / 60;

                return returnValue;
            }
            catch (Exception ex)
            {
                var message = string.Format("Error getting degrees from DM/Dir of {0}/{1}", DM, Dir);
                throw new Exception(message, ex);
            }
        }

        public GpsConnection(string portName, GpsState gpsState)
        {
            GpsState = gpsState;
            _SerialPort = new SerialPort
            {
                PortName = portName,
                BaudRate = 4800,
                Parity = Parity.None,
                DataBits = 8,
                StopBits = StopBits.One
            };
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
                    GpsState.Latitude = GetDegrees(fields[2], fields[3]);
                    GpsState.Longitude = GetDegrees(fields[4], fields[5]);
                    break;
                case "$GPGSA":
                    break;
                case "$GPGSV":
                    break;
                case "$GPRMC":
                    double speed;
                    if (double.TryParse(fields[7], out speed))
                        GpsState.Speed = speed;
                    else
                        GpsState.Speed = 0;
                    double heading;
                    if (double.TryParse(fields[8], out heading))
                        GpsState.Heading = heading;
                    else
                        GpsState.Heading = 0;
                    break;
            }
        }

        public GpsState GpsState { get; private set; }

        private bool _IsOpen;
        public bool IsOpen
        {
            get { return _IsOpen; }
        }

        readonly SerialPort _SerialPort;

        public void Open()
        {
            _SerialPort.DataReceived += SerialPort_DataReceived;
            _SerialPort.Open();

            //// Switch from SiRF Binary to NMEA Output
            //string str = "A0 A2 00 18 81 02 01 01 00 01 01 01 05 01 01 01 00 01 00 01 00 00 00 01 00 00 12 C0 01 65 B0 B3";
            //byte[] bytes = str.Split(' ').Select(s => Convert.ToByte(s, 16)).ToArray();
            //_SerialPort.Write(bytes, 0, bytes.Length);
        }
    }
}
