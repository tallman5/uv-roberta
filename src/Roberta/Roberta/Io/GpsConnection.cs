using Roberta.ComponentModel;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;

namespace Roberta.Io
{
    public class GpsConnection : Item, IConnection
    {
        public void Close()
        {
        }

        public void Dispose()
        {
        }

        public GpsConnection(string sensorPath)
        {
            _SensorPath = sensorPath;
        }

        public void Open()
        {
        }

        private string _SensorPath;
        [DataMember]
        public virtual string SensorPath
        {
            get { return this._SensorPath; }
            set
            {
                if (value != this._SensorPath)
                {
                    this._SensorPath = value;
                    this.RaisePropertyChanged(nameof(SensorPath));
                }
            }
        }
    }
}
