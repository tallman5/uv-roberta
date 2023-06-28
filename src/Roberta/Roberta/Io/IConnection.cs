namespace Roberta.Io
{
    public interface IConnection : IDisposable
    {
        void Close();
        string ConnectionString { get; }
        bool IsOpen { get; }
        void Open();
    }
}
