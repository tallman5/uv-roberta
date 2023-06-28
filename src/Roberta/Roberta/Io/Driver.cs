using Roberta.ComponentModel;
using System.Runtime.Serialization;

namespace Roberta.Io
{
    public class Driver : Item
    {
        private string _ConnectionId;
        [DataMember]
        public string ConnectionId
        {
            get { return _ConnectionId; }
            set
            {
                if (_ConnectionId != value)
                {
                    _ConnectionId = value;
                    RaisePropertyChanged(nameof(ConnectionId));
                }
            }
        }

        public Driver()
        {
            _ConnectionId = string.Empty;
            _ScreenName = string.Empty;
            _UserName = string.Empty;

        }

        private DriverStatusType _DriverStatusType;
        [DataMember]
        public DriverStatusType DriverStatusType
        {
            get { return _DriverStatusType; }
            set
            {
                if (_DriverStatusType != value)
                {
                    _DriverStatusType = value;
                    RaisePropertyChanged(nameof(DriverStatusType));
                }
            }
        }

        private string _ScreenName;
        [DataMember]
        public string ScreenName
        {
            get { return _ScreenName; }
            set
            {
                if (_ScreenName != value)
                {
                    _ScreenName = value;
                    RaisePropertyChanged(nameof(ScreenName));
                }
            }
        }

        private string _UserName;
        [DataMember]
        public string UserName
        {
            get { return _UserName; }
            set
            {
                if (_UserName != value)
                {
                    _UserName = value;
                    RaisePropertyChanged(nameof(UserName));
                }
            }
        }
    }
}
