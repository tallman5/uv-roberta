using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Runtime.Serialization;

namespace Roberta.Io
{
    public class RxState : ItemState
    {
        private const int CHANNELS = 16;

        public RxState()
        {
            this.ChannelValues = new ObservableCollection<int>();
            for (int i = 0; i < CHANNELS; i++)
                this.ChannelValues.Add(0);
            this.ChannelValues.CollectionChanged += ChannelValues_CollectionChanged;
        }

        private void ChannelValues_CollectionChanged(object? sender, NotifyCollectionChangedEventArgs e)
        {
            if (ChannelValues.Count != CHANNELS) throw new InvalidOperationException($"Channel values must contain exactly {CHANNELS} items.");
            Timestamp = DateTimeOffset.Now;
            RaisePropertyChanged();
        }

        [DataMember]
        public ObservableCollection<int> ChannelValues { get; set; }

        [DataMember]
        private bool _InFailsafe;
        public bool InFailsafe
        {
            get { return _InFailsafe; }
            set
            {
                if (value != _InFailsafe)
                {
                    _InFailsafe = value;
                    Timestamp = DateTimeOffset.Now;
                    RaisePropertyChanged(nameof(InFailsafe));
                }
            }
        }

        [DataMember]
        private bool _Ch17;
        public bool Ch17
        {
            get { return _Ch17; }
            set
            {
                if (value != _Ch17)
                {
                    _Ch17 = value;
                    Timestamp = DateTimeOffset.Now;
                    RaisePropertyChanged(nameof(Ch17));
                }
            }
        }

        [DataMember]
        private bool _Ch18;
        public bool Ch18
        {
            get { return _Ch18; }
            set
            {
                if (value != _Ch18)
                {
                    _Ch18 = value;
                    Timestamp = DateTimeOffset.Now;
                    RaisePropertyChanged(nameof(Ch18));
                }
            }
        }
    }
}
