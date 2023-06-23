using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Reflection;
using System.Runtime.Serialization;

namespace Roberta.Io
{

    public class RxState : ItemState
    {
        public RxState()
        {
            this.ChannelValues = new ObservableCollection<int> { 0, 0, 0, 0, 0, 0, 0 };
            this.ChannelValues.CollectionChanged += ChannelValues_CollectionChanged;
        }

        private void ChannelValues_CollectionChanged(object? sender, NotifyCollectionChangedEventArgs e)
        {
            RaisePropertyChanged();
            //Console.WriteLine(e?.Action);
            //Console.WriteLine(e?.NewItems?.Count);
            //Console.WriteLine(e?.NewStartingIndex);
            //Console.WriteLine(e?.OldItems?.Count);
            //Console.WriteLine(e?.OldStartingIndex);
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
        public bool InFailsafe
        {
            get
            {
                bool returnValue = true;
                foreach (var cv in ChannelValues)
                {
                    if (cv != 0)
                    {
                        returnValue = false;
                        break;
                    }
                }
                return returnValue;
            }
        }

        [DataMember]
        public bool IsArmed
        {
            get
            {
                bool returnValue = false;
                if (ChannelValues[4] > 1800) returnValue = true;

                return returnValue;
            }
        }
    }
}
