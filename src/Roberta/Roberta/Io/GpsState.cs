using System;
using System.IO.Ports;
using System.Runtime.Serialization;

namespace Roberta.Io
{
    public class GpsState : ItemState
    {
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
    }
}
