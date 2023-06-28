using System.Runtime.Serialization;

namespace Roberta.Io
{
    public class GpsMixerState : GpsState
    {
        private GpsState _LeftState;
        public GpsState LeftState
        {
            get { return _LeftState; }
            set
            {
                _LeftState = value;
            }
        }

        private GpsState _RightState;
        public GpsState RightState
        {
            get { return _RightState; }
            set
            {
                _RightState = value;
            }
        }

        public GpsMixerState(GpsState leftState, GpsState rightState)
        {
            _LeftState = leftState;
            _LeftState.PropertyChanged += GpsState_PropertyChanged;
            _RightState = rightState;
            _RightState.PropertyChanged += GpsState_PropertyChanged;
        }

        private void GpsState_PropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            bool ready = false;

            if (_LeftState.IsReady && _RightState.IsReady)
            {
                _Latitude = (_LeftState.Latitude + _RightState.Latitude) / 2;
                _Longitude = (_LeftState.Longitude + _RightState.Longitude) / 2;
                _Speed = (_LeftState.Speed + _RightState.Speed) / 2;

                double lat1Rad = _LeftState.Latitude * Math.PI / 180.0;
                double lon1Rad = _LeftState.Longitude * Math.PI / 180.0;
                double lat2Rad = _RightState.Latitude * Math.PI / 180.0;
                double lon2Rad = _RightState.Longitude * Math.PI / 180.0;

                double deltaLonRad = lon2Rad - lon1Rad;
                double heading = Math.Atan2(Math.Sin(deltaLonRad), Math.Cos(lat2Rad) * Math.Tan(lat1Rad) - Math.Sin(lat2Rad) * Math.Cos(deltaLonRad));

                // Convert the heading from radians to degrees
                _Heading = heading * 180.0 / Math.PI;

                ready = true;
            }

            this.IsReady = ready;
            RaisePropertyChanged();
        }

        [DataMember]
        public double Distance
        {
            get
            {
                return Utilities.CalculateDistance(_LeftState.Latitude, _LeftState.Longitude,
                    _RightState.Latitude, _RightState.Longitude);
            }
        }
    }
}
