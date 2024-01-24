namespace Roberta.Io
{
    public class PowerMixer
    {
        static public PowerMixer FromLR(decimal l, decimal r)
        {
            var returnValue = new PowerMixer();
            returnValue.Left = l;
            returnValue.Right = r;
            return returnValue;
        }

        static public PowerMixer FromXY(decimal x, decimal y)
        {
            var returnValue = new PowerMixer();
            returnValue._X = x;
            returnValue._Y = y;
            returnValue.UpdateLR();
            return returnValue;
        }

        private decimal GetSafeValue(decimal newValue)
        {
            decimal returnValue = Math.Max(newValue, -1000);
            returnValue = Math.Min(returnValue, 1000);
            return returnValue;
        }

        private decimal _Left;
        public decimal Left
        {
            get { return _Left; }
            set
            {
                this._Left = GetSafeValue(value);
                this.UpdateXY();
            }
        }

        private decimal _Right;
        public decimal Right
        {
            get { return this._Right; }
            set
            {
                this._Right = GetSafeValue(value);
                this.UpdateXY();
            }
        }

        private void UpdateLR()
        {
            this._Left = this.GetSafeValue(this._X + this._Y);
            this._Right = this.GetSafeValue(this._Y - this._X);
        }

        private void UpdateXY()
        {
        }

        private decimal _X;
        public decimal X
        {
            get { return this._X; }
            set
            {
                this._X = GetSafeValue(value);
                this.UpdateLR();
            }
        }

        private decimal _Y;
        public decimal Y
        {
            get { return this._Y; }
            set
            {
                this._Y = GetSafeValue(value);
                this.UpdateLR();
            }
        }
    }
}
