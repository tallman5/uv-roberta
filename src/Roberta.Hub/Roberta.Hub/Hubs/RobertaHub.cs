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
            Console.WriteLine($"Echo Message: {message}");
            await Clients.All.Echo(message);
        }

        public override Task OnConnectedAsync()
        {
            Console.WriteLine("New Connection");
            return base.OnConnectedAsync();
        }

        public async Task UpdateGpsState(GpsState gpsState)
        {
            Console.WriteLine("Updating GPS State...");
            await Clients.All.GpsStateUpdated(gpsState);
        }

        public async Task UpdateRxState(RxState rxState)
        {
            Console.WriteLine("Updating RX State...");
            await Clients.All.RxStateUpdated(rxState);
        }
    }
}
