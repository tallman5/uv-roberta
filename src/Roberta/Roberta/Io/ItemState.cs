using Roberta.ComponentModel;
using System;
using System.Collections.Generic;
using System.Runtime.Serialization;
using System.Text;

namespace Roberta.Io
{
    public interface IItemState
    {
        DateTimeOffset Timestamp { get; set; }
    }

    public class ItemState : Item, IItemState
    {
        public ItemState()
        {
            this._IsReady = false;
            this._Timestamp = DateTime.Now;
            this.PropertyChanged += ItemState_PropertyChanged;
        }

        private void ItemState_PropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            if (e.PropertyName != "Timestamp")
                this._Timestamp = DateTimeOffset.Now;
        }

        private bool _IsReady;
        [DataMember]
        public virtual bool IsReady
        {
            get { return this._IsReady; }
            internal set
            {
                if (this._IsReady != value)
                {
                    this._IsReady = value;
                    RaisePropertyChanged(nameof(IsReady));
                }
            }
        }

        private DateTimeOffset _Timestamp;
        [DataMember]
        public virtual DateTimeOffset Timestamp
        {
            get { return this._Timestamp; }
            set
            {
                if (this._Timestamp != value)
                    this._Timestamp = value;
            }
        }
    }
}
