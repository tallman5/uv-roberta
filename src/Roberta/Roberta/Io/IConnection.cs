namespace Roberta.Io
{
    public interface IConnection : IDisposable
    {
        void Close();
        bool IsOpen { get; }
        void Open();
    }
}
