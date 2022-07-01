using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;

namespace Roberta.Io
{
    public class RoboteqState : ItemState
    {
        private BatteryState _BatteryState;
        [DataMember]
        public BatteryState BatteryState
        {
            get { return this._BatteryState; }
            set
            {
                if (value != this._BatteryState)
                {
                    this._BatteryState = value;
                    RaisePropertyChanged("BatteryState");
                }
            }
        }

        private CommandPriority _CommandPriority;
        [DataMember]
        public CommandPriority CommandPriority
        {
            get { return this._CommandPriority; }
            set
            {
                if (value != this._CommandPriority)
                {
                    this._CommandPriority = value;
                    RaisePropertyChanged("CommandPriority");
                }
            }
        }

        private MotorState _LeftMotorState;
        [DataMember]
        public MotorState LeftMotorState
        {
            get { return this._LeftMotorState; }
            set
            {
                if (value != this._LeftMotorState)
                {
                    this._LeftMotorState = value;
                    RaisePropertyChanged("LeftMotorState");
                }
            }
        }

        private decimal _MainTemperature;
        [DataMember]
        public decimal MainTemperature
        {
            get { return this._MainTemperature; }
            set
            {
                if (value != this._MainTemperature)
                {
                    this._MainTemperature = value;
                    RaisePropertyChanged("MainTemperature");
                }
            }
        }

        private MixMode _MixMode;
        [DataMember]
        public MixMode MixMode
        {
            get { return this._MixMode; }
            set
            {
                if (value != this._MixMode)
                {
                    this._MixMode = value;
                    RaisePropertyChanged("MixMode");
                }
            }
        }

        private MotorState _RightMotorState;
        [DataMember]
        public MotorState RightMotorState
        {
            get { return this._RightMotorState; }
            set
            {
                if (value != this._RightMotorState)
                {
                    this._RightMotorState = value;
                    RaisePropertyChanged("RightMotorState");
                }
            }
        }

        public RoboteqState()
        {
            _BatteryState = new BatteryState();
            _BatteryState.PropertyChanged += ChildState_PropertyChanged;
            _LeftMotorState = new MotorState();
            _LeftMotorState.PropertyChanged += ChildState_PropertyChanged;
            _RightMotorState = new MotorState();
            _RightMotorState.PropertyChanged += ChildState_PropertyChanged;
        }

        private void ChildState_PropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            RaisePropertyChanged();
        }
    }

    public enum CommandPriority
    { Application, Transmitter }

    public enum MixMode
    { Separate, Mode1, Mode2 }
}
