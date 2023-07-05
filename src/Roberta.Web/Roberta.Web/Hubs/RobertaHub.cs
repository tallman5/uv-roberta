using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.SignalR;
using Roberta.Io;
using System.Collections.ObjectModel;

namespace Roberta.Web.Hubs
{
    public class RobertaHub : Hub<IRobertaClient>
    {
        public override Task OnConnectedAsync()
        {
            return base.OnConnectedAsync();
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
