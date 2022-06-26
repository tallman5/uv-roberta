using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace Roberta.Win.Input
{
    public class Command : ICommand
    {
        readonly Action _Action;

        bool _CanExecute;
        public bool CanExecute
        {
            get { return this._CanExecute; }
            set
            {
                if (value != this._CanExecute)
                {
                    this._CanExecute = value;
                    CanExecuteChanged?.Invoke(this, EventArgs.Empty);
                }
            }
        }

        bool ICommand.CanExecute(object? parameter)
        {
            return _CanExecute;
        }

        public event EventHandler? CanExecuteChanged;

        public Command(Action action, bool canExecute = true)
        {
            this._Action = action;
            this._CanExecute = canExecute;
        }

        public void Execute(object? parameter)
        {
            if (null != this._Action)
            {
                this._Action();
                return;
            }
        }
    }
}
