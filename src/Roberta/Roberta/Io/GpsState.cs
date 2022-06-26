using System;
using System.IO.Ports;
using System.Runtime.Serialization;

namespace Roberta.Io
{
    public class GpsState : ItemState
    {
        static public GpsState Create()
        {
            var returnValue = new GpsState
            {
                Title = "GPS State"
            };
            return returnValue;
        }

        public static SerialPort CreatePort(string portName)
        {
            var returnValue = new SerialPort
            {
                PortName = portName,
                BaudRate = 4800,
                Parity = Parity.None,
                DataBits = 8,
                StopBits = StopBits.One
            };
            return returnValue;
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

        private double _Heading;
        [DataMember]
        public virtual double Heading
        {
            get { return this._Heading; }
            set
            {
                if (this._Heading != value)
                {
                    this._Heading = value;
                    RaisePropertyChanged("Heading");
                }
            }
        }

        private double _Latitude;
        [DataMember]
        public virtual double Latitude
        {
            get { return this._Latitude; }
            set
            {
                if (this._Latitude != value)
                {
                    this._Latitude = value;
                    RaisePropertyChanged("Latitude");
                }
            }
        }

        private double _Longitude;
        [DataMember]
        public virtual double Longitude
        {
            get { return this._Longitude; }
            set
            {
                if (this._Longitude != value)
                {
                    this._Longitude = value;
                    RaisePropertyChanged("Longitude");
                }
            }
        }

        private double _Speed;
        [DataMember]
        public virtual double Speed
        {
            get { return this._Speed; }
            set
            {
                if (this._Speed != value)
                {
                    this._Speed = value;
                    RaisePropertyChanged("Speed");
                }
            }
        }

        static public void Update(GpsState state, string sentence)
        {
            bool isValid = false;
            string sentenceToCheck = "";

            try
            {
                sentenceToCheck = sentence.Trim();
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
                    state.Latitude = GetDegrees(fields[2], fields[3]);
                    state.Longitude = GetDegrees(fields[4], fields[5]);
                    break;
                case "$GPGSA":
                    break;
                case "$GPGSV":
                    break;
                case "$GPRMC":
                    double speed;
                    if (double.TryParse(fields[7], out speed))
                        state.Speed = speed;
                    double heading;
                    if (double.TryParse(fields[8], out heading))
                        state.Heading = heading;
                    break;
            }
        }
    }
}
