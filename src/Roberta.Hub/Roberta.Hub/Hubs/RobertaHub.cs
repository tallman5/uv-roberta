using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.SignalR;
using Roberta.Io;

namespace Roberta.Hub.Hubs
{
    [AllowAnonymous]
    public class RobertaHub : Hub<IRobertaClient>
    {
        static DateTimeOffset lastGpsTs = DateTimeOffset.MinValue;
        static DateTimeOffset lastRoboteqTs = DateTimeOffset.MinValue;
        static DateTimeOffset lastRxTs = DateTimeOffset.MinValue;
        static DateTimeOffset lastThumbstickTs = DateTimeOffset.MinValue;

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
