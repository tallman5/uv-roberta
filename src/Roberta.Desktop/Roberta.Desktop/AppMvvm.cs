using Roberta.ComponentModel;
using Roberta.Io;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Reflection;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;

namespace Roberta.Desktop
{
    internal class AppMvvm : Item, IDisposable
    {
        public AppMvvm()
        {
            _Connection = new RoboteqConnection(App.Current);
        }

        private RoboteqConnection _Connection;
        [DataMember]
        public virtual RoboteqConnection Connection
        {
            get { return this._Connection; }
            set
            {
                if (value != this._Connection)
                {
                    this._Connection = value;
                    this.RaisePropertyChanged(nameof(Connection));
                }
            }
        }

        public void Dispose()
        {
            if (null != _Connection)
            {
                _Connection.Dispose();
            }
        }
    }
}
