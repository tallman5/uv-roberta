using System;
using System.IO.Ports;
using System.Runtime.Serialization;

namespace Roberta.Io
{
    public class GpsState : ItemState
    {
        protected double _Heading;
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

        protected double _Latitude;
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

        protected double _Longitude;
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

        /// <summary>
        /// One NMEA line has all values, set all before firing prop changed
        /// </summary>
        /// <param name="lat"></param>
        /// <param name="lon"></param>
        /// <param name="speed"></param>
        /// <param name="heading"></param>
        internal void SetVals(double lat, double lon, double speed, double heading)
        {
            _Latitude = lat;
            _Longitude = lon;
            _Speed = speed;
            _Heading = heading;
            Timestamp = DateTimeOffset.Now;
            RaisePropertyChanged();
        }

        protected double _Speed;
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
    }
}
