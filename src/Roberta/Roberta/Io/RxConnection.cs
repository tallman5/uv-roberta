using Roberta.Win.Input;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Roberta.Io
{
    public class RxConnection : ItemState, IDisposable
    {
        private bool _IsOpen;
        private RxState _RxState;

        public virtual void Close()
        {
            if (_IsOpen)
            {
                _IsOpen = false;
                CloseCommand.CanExecute = false;
                OpenCommand.CanExecute = true;
            }
        }
        public Command CloseCommand { get; private set; }

        public void Dispose()
        {
        }

        public virtual void Open()
        {
            if (!_IsOpen)
            {
                _IsOpen = true;
                CloseCommand.CanExecute = true;
                OpenCommand.CanExecute = false;
            }
        }
        public Command OpenCommand { get; private set; }

        public RxConnection(RxState rxState)
        {
            _IsOpen = false;
            _RxState = rxState;
            CloseCommand = new Command(Close, false);
            OpenCommand = new Command(Open, true);
        }
    }
}
