using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;

namespace Roberta.ComponentModel
{
    public class NotifyPropertyChanged : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler? PropertyChanged;

        public virtual void RaisePropertyChanged()
        {
            // Raise property changed for all properties
            // Helpful when control bound to calculated properties with no set

            var t = this.GetType();
            TypeInfo ti = t.GetTypeInfo();
            foreach (var prop in ti.DeclaredProperties)
                this.RaisePropertyChanged(prop.Name);
        }

        public virtual void RaisePropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
