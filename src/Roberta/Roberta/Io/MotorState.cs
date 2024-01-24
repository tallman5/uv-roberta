using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;

namespace Roberta.Io
{
    public class MotorState : ItemState
    {
        private decimal _BatteryAmps;
        [DataMember]
        public decimal BatteryAmps
        {
            get { return this._BatteryAmps; }
            set
            {
                if (value != this._BatteryAmps)
                {
                    this._BatteryAmps = value;
                    RaisePropertyChanged("BatteryAmps");
                }
            }
        }

        private decimal _DesiredPower;
        [DataMember]
        public decimal DesiredPower
        {
            get { return this._DesiredPower; }
            set
            {
                if (value != this._DesiredPower)
                {
                    this._DesiredPower = value;
                    RaisePropertyChanged("DesiredPower");
                }
            }
        }

        private decimal _MotorAmps;
        [DataMember]
        public decimal MotorAmps
        {
            get { return this._MotorAmps; }
            set
            {
                if (value != this._MotorAmps)
                {
                    this._MotorAmps = value;
                    RaisePropertyChanged("MotorAmps");
                }
            }
        }

        private decimal _Power;
        [DataMember]
        public decimal Power
        {
            get { return this._Power; }
            set
            {
                if (value != this._Power)
                {
                    this._Power = value;
                    RaisePropertyChanged("Power");
                }
            }
        }

        private decimal _Temperature;
        [DataMember]
        public decimal Temperature
        {
            get { return this._Temperature; }
            set
            {
                if (value != this._Temperature)
                {
                    this._Temperature = value;
                    RaisePropertyChanged("Temperature");
                }
            }
        }
    }
}
