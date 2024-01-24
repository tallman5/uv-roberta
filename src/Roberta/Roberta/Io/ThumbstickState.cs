using System.Runtime.Serialization;

namespace Roberta.Io
{
    public class ThumbstickState : ItemState
    {
        [DataMember]
        private int _X;
        public int X
        {
            get { return _X; }
            set
            {
                if (value != _X)
                {
                    _X = value;
                    Timestamp = DateTimeOffset.Now;
                    RaisePropertyChanged(nameof(X));
                }
            }
        }

        [DataMember]
        private int _Y;
        public int Y
        {
            get { return _Y; }
            set
            {
                if (value != _Y)
                {
                    _Y = value;
                    Timestamp = DateTimeOffset.Now;
                    RaisePropertyChanged(nameof(Y));
                }
            }
        }
    }
}
