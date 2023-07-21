using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.SignalR;
using Roberta.Io;

namespace Roberta.Hub.Hubs
{
    [AllowAnonymous]
    public class RobertaHub : Hub<IRobertaClient>
    {
        public async void Echo(string message)
        {
            await Clients.All.Echo(message);
        }

        public override Task OnConnectedAsync()
        {
            return base.OnConnectedAsync();
        }

        public override Task OnDisconnectedAsync(Exception? exception)
        {
            return base.OnDisconnectedAsync(exception);
        }

        public async Task UpdateGpsState(GpsState gpsState)
        {
            await Clients.All.GpsStateUpdated(gpsState);
        }

        public async Task UpdateRxState(RxState rxState)
        {
            await Clients.All.RxStateUpdated(rxState);
        }
    }
}
