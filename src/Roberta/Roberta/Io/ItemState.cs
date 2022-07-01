using Roberta.ComponentModel;
using System;
using System.Collections.Generic;
using System.Runtime.Serialization;
using System.Text;

namespace Roberta.Io
{
    [DataContract(Namespace = "http://schemas.mcgurkin.net/2004/07/Roberta.Hardware")]
    public class ItemState : Item
    {
        public ItemState()
        {
            this._Timestamp = DateTime.Now;
            this.PropertyChanged += ItemState_PropertyChanged;
        }

        private void ItemState_PropertyChanged(object sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            if (e.PropertyName != "Timestamp")
                this._Timestamp = DateTimeOffset.Now;
        }

        private DateTimeOffset _Timestamp;
        [DataMember]
        public virtual DateTimeOffset Timestamp
        {
            get { return this._Timestamp; }
            set
            {
                if (this._Timestamp != value)
                {
                    this._Timestamp = value;
                    //RaisePropertyChanged("Timestamp");
                }
            }
        }
    }
}
