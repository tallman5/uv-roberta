using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Runtime.Serialization;

namespace Roberta.Io
{
    public enum PilotMode
    {
        Transmitter, Mission, Other
    }

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
            RaisePropertyChanged();
            if (ChannelValues.Count != CHANNELS) throw new InvalidOperationException($"Channel values must contain exactly {CHANNELS} items.");
        }

        [DataMember]
        public ObservableCollection<int> ChannelValues { get; private set; }

        public int Channel01
        {
            get { return this.ChannelValues[0]; }
        }

        public int Channel02
        {
            get { return this.ChannelValues[1]; }
        }

        public int Channel03
        {
            get { return this.ChannelValues[2]; }
        }

        public int Channel04
        {
            get { return this.ChannelValues[3]; }
        }

        public int Channel05
        {
            get { return this.ChannelValues[4]; }
        }

        public int Channel06
        {
            get { return this.ChannelValues[5]; }
        }

        public int Channel07
        {
            get { return this.ChannelValues[6]; }
        }

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
                    RaisePropertyChanged(nameof(InFailsafe));
                }
            }
        }

        [DataMember]
        public bool IsArmed
        {
            get
            {
                if (!IsReady) return false;
                bool returnValue = (ChannelValues[4] > 1600) ? true : false;
                return returnValue;
            }
        }

        [DataMember]
        public PilotMode PilotMode
        {
            get
            {
                PilotMode returnValue = PilotMode.Transmitter;
                if (ChannelValues[5] > -20000) returnValue = PilotMode.Mission;
                if (ChannelValues[5] > 20000) returnValue = PilotMode.Other;
                return returnValue;
            }
        }
    }
}
