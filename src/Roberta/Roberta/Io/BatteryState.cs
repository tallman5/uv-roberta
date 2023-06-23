using Roberta.ComponentModel;
using System.Runtime.Serialization;

namespace Roberta.Io
{
    public class BatteryState : Item
    {
        private const decimal DEF_BATT_SCALE = 1.125M;
        private const decimal DEF_SYS_VOLTS = 24M;

        public BatteryState()
        {
            _MaxVoltage = DEF_SYS_VOLTS;
        }

        private decimal _DangerLevel;
        /// <summary>
        /// Based on system volts and scale, estimated level when indicators should show red
        /// </summary>
        [DataMember]
        public decimal DangerLevel
        {
            get { return this._DangerLevel; }
            set
            {
                if (value != this._DangerLevel)
                {
                    this._DangerLevel = value;
                    this.RaisePropertyChanged(nameof(DangerLevel));
                }
            }
        }

        private decimal _MaxVoltage;
        /// <summary>
        /// Based on system volts and scale, estimated max battery voltage
        /// </summary>
        [DataMember]
        public decimal MaxVoltage
        {
            get { return this._MaxVoltage; }
            set
            {
                if (value != this._MaxVoltage)
                {
                    this._MaxVoltage = value;
                    this.RaisePropertyChanged(nameof(MaxVoltage));
                }
            }
        }

        private decimal _MinVoltage;
        /// <summary>
        /// Based on system volts and scale, estimated min battery voltage
        /// </summary>
        [DataMember]
        public decimal MinVoltage
        {
            get { return this._MinVoltage; }
            set
            {
                if (value != this._MinVoltage)
                {
                    this._MinVoltage = value;
                    this.RaisePropertyChanged(nameof(MinVoltage));
                }
            }
        }

        private int _Percent;
        /// <summary>
        /// Based on system volts, min and scale, estimated battery remaining percent.
        /// This varies based on load, not meant to be a "real" level indicator
        /// </summary>
        [DataMember]
        public int Percent
        {
            get { return this._Percent; }
            set
            {
                if (value != this._Percent)
                {
                    this._Percent = value;
                    this.RaisePropertyChanged(nameof(Percent));
                }
            }
        }

        private decimal _SystemVolts;
        /// <summary>
        /// The voltage of the batteries connected, eg, 24 for two 12v batteries in series
        /// </summary>
        [DataMember]
        public decimal SystemVolts
        {
            get { return this._SystemVolts; }
            set
            {
                if (value != this._SystemVolts)
                {
                    this._SystemVolts = value;
                    this._MaxVoltage = value * DEF_BATT_SCALE;
                    this._MinVoltage = value / DEF_BATT_SCALE;
                    this._DangerLevel = this.MinVoltage * (DEF_BATT_SCALE / 2);
                    RaisePropertyChanged(nameof(SystemVolts));
                }
            }
        }

        private decimal _Volts;
        /// <summary>
        /// Actual voltage read from controller
        /// </summary>
        [DataMember]
        public decimal Volts
        {
            get { return this._Volts; }
            set
            {
                if (value != this._Volts)
                {
                    this._Volts = value;
                    RaisePropertyChanged(nameof(Volts));
                }
            }
        }
    }
}
