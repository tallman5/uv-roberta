using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.SignalR;
using Roberta.Io;

namespace Roberta.Hub.Hubs
{
    [AllowAnonymous]
    public class RobertaHub : Hub<IRobertaClient>
    {
        DateTimeOffset lastGpsTs;
        DateTimeOffset lastRoboteqTs;
        DateTimeOffset lastRxTs;
        DateTimeOffset lastThumbstickTs;

        public RobertaHub()
        {
            Console.Write("Initializing Roberta Hub...");
            lastGpsTs = DateTimeOffset.MinValue;
            lastRoboteqTs = DateTimeOffset.MinValue;
            lastRxTs = DateTimeOffset.MinValue;
            lastThumbstickTs = DateTimeOffset.MinValue;
            Console.WriteLine("done");
        }

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
            if (gpsState.Timestamp > lastGpsTs)
            {
                await Clients.All.GpsStateUpdated(gpsState);
                lastGpsTs = gpsState.Timestamp;
            }
        }

        public async Task UpdateRoboteqState(RoboteqState roboteqState)
        {
            if (roboteqState.Timestamp > lastRoboteqTs)
            {
                await Clients.All.RoboteqStateUpdated(roboteqState);
                lastRoboteqTs = roboteqState.Timestamp;
            }
        }

        public async Task UpdateRxState(RxState rxState)
        {
            if (rxState.Timestamp > lastRxTs)
            {
                await Clients.All.RxStateUpdated(rxState);
                lastRxTs = rxState.Timestamp;
            }
        }

        public async Task UpdateThumbstickState(ThumbstickState thumbstickState)
        {
            if (thumbstickState.Timestamp > lastThumbstickTs)
            {
                await Clients.All.ThumbstickStateUpdated(thumbstickState);
                lastThumbstickTs = thumbstickState.Timestamp;
            }
        }
    }
}
