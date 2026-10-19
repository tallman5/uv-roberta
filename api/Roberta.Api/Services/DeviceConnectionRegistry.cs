namespace Roberta.Api.Services;

public sealed class DeviceConnectionRegistry
{
    private readonly object gate = new();
    private string? connectionId;

    public bool Register(string id)
    {
        lock (gate)
        {
            if (connectionId is not null)
            {
                return false;
            }

            connectionId = id;
            return true;
        }
    }

    public void Unregister(string id)
    {
        lock (gate)
        {
            if (connectionId == id)
            {
                connectionId = null;
            }
        }
    }

    public string? GetConnectionId()
    {
        lock (gate)
        {
            return connectionId;
        }
    }
}
