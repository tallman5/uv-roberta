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
        private const int CHANNELS = 7;

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

        public ObservableCollection<int> ChannelValues { get; private set; }

        [DataMember]
        public int Channel01
        {
            get { return this.ChannelValues[0]; }
        }

        [DataMember]
        public int Channel02
        {
            get { return this.ChannelValues[1]; }
        }

        [DataMember]
        public int Channel03
        {
            get { return this.ChannelValues[2]; }
        }

        [DataMember]
        public int Channel04
        {
            get { return this.ChannelValues[3]; }
        }

        [DataMember]
        public int Channel05
        {
            get { return this.ChannelValues[4]; }
        }

        [DataMember]
        public int Channel06
        {
            get { return this.ChannelValues[5]; }
        }

        [DataMember]
        public int Channel07
        {
            get { return this.ChannelValues[6]; }
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
