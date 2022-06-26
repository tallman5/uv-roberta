using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Roberta.Io
{
    public interface IConnection : IDisposable
    {
        public void Close();
        public void Open();
        public string SensorPath { get; set; }
    }
}
