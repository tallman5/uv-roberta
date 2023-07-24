using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.SignalR;
using Roberta.Io;
using System.Collections.ObjectModel;

namespace Roberta.Hub.Hubs
{
    [Authorize(AuthenticationSchemes = Utilities.AUTH_SCHEMES)]
    public class RobertaHub : Hub<IRobertaClient>
    {
        // TODO: remove static _Drivers when app goes distributed
        // TODO: Ensure SetXY is coming from current driver

        const string RobertaGroup = "RobertaGroup";
        private static ObservableCollection<Driver> _Drivers;
        private static GpsState _LastGpsState;
        private static RoboteqState _LastRoboteqState;
        private static ThumbstickState _LastThumstickState;

        public RobertaHub()
        {
            if (null == _Drivers) _Drivers = new();
            if (null == _LastGpsState) _LastGpsState = new();
            if (null == _LastRoboteqState) _LastRoboteqState = new();
            if (null == _LastThumstickState) _LastThumstickState = new();
        }

        public async void Echo(string message)
        {
            await Clients.All.Echo(message);
        }

        public async Task Iam(string screenName)
        {
            var existingDriver = _Drivers.Where(d => d.ConnectionId == Context.ConnectionId).FirstOrDefault();
            if (null != existingDriver)
            {
                existingDriver.ScreenName = screenName;
                await Clients.All.DriversUpdated(_Drivers);
            }
        }

        public override Task OnConnectedAsync()
        {
            if (Context.User.IsInRole("Roberta"))
            {
                Groups.AddToGroupAsync(Context.ConnectionId, RobertaGroup);
            }
            else
            {
                var userName = Context.User.Identity.Name;
                if (string.IsNullOrWhiteSpace(userName) || userName == "guest@mcgurkin.net")
                    userName = Context.ConnectionId;

                var existingDriver = _Drivers.Where(d => d.UserName == userName).FirstOrDefault();
                if (null == existingDriver)
                {
                    var newDriver = new Driver
                    {
                        ConnectionId = Context.ConnectionId,
                        DriverStatusType = DriverStatusTypes.Waiting,
                        ScreenName = userName,
                        Title = "Driver",
                        UserName = userName
                    };

                    if (newDriver.ScreenName == "joseph@mcgurkin.net")
                    {
                        newDriver.ScreenName = "tallman";
                        _Drivers.Insert(0, newDriver);
                    }
                    else
                    {
                        _Drivers.Add(newDriver);
                    }
                }
            }

            Clients.All.DriversUpdated(_Drivers);
            Clients.Caller.GpsStateUpdated(_LastGpsState);
            Clients.Caller.RoboteqStateUpdated(_LastRoboteqState);
            Clients.Caller.ThumbstickStateUpdated(_LastThumstickState);

            return base.OnConnectedAsync();
        }

        public override Task OnDisconnectedAsync(Exception? exception)
        {
            var existingDriver = _Drivers.Where(d => d.ConnectionId == Context.ConnectionId).FirstOrDefault();
            if (null != existingDriver)
            {
                _Drivers.Remove(existingDriver);
                Clients.All.DriversUpdated(_Drivers);
            }
            //if (Context.User.IsInRole("Roberta"))
            //{
            //    _LastGpsState.IsOnline = false;
            //    _LastRoboteqState.IsOnline = false;

            //    Clients.Caller.GpsStateUpdated(_LastGpsState);
            //    Clients.Caller.RoboteqStateUpdated(_LastRoboteqState);
            //}
            return base.OnDisconnectedAsync(exception);
        }

        private async Task ResetDrivers()
        {
            await Clients.All.StopDriving();
            await Clients.Group(RobertaGroup).SetXY(0, 0);

            var currentDrivers = _Drivers.Where(d => d.DriverStatusType.Id == DriverStatusTypes.Driving.Id);
            bool changed = false;
            foreach (var currentDriver in currentDrivers)
            {
                currentDriver.DriverStatusType = DriverStatusTypes.Waiting;
                changed = true;
            }
            if (changed)
                await Clients.All.DriversUpdated(_Drivers);
        }

        [Authorize(Roles = "Administrator")]
        public async Task SetPowerScale(decimal powerScale)
        {
            await Clients.Group(RobertaGroup).SetPowerScale(powerScale);
        }

        public async Task SetXY(decimal x, decimal y)
        {
            await Clients.Group(RobertaGroup).SetXY(x, y);
        }

        [Authorize(Roles = "Administrator")]
        public async Task StartDriving(string connectionId)
        {
            await StopDriving();

            var newDriver = _Drivers.Where(d => d.ConnectionId == connectionId).FirstOrDefault();
            if (null != newDriver)
            {
                newDriver.DriverStatusType = DriverStatusTypes.Driving;
                await Clients.Clients(connectionId).StartDriving(string.Empty);
                await Clients.All.DriversUpdated(_Drivers);
            }
        }

        [Authorize(Roles = "Administrator")]
        public async Task StopDriving()
        {
            await ResetDrivers();
            //await SetCommandPriority(CommandPriority.Transmitter);
        }

        public async Task UpdateGpsState(GpsState gpsState)
        {
            if (gpsState.Timestamp > _LastGpsState.Timestamp)
            {
                await Clients.All.GpsStateUpdated(gpsState);
                _LastGpsState = gpsState;
            }
        }

        public async Task UpdateRoboteqState(RoboteqState roboteqState)
        {
            if (roboteqState.Timestamp > _LastRoboteqState.Timestamp)
            {
                await Clients.All.RoboteqStateUpdated(roboteqState);
                _LastRoboteqState = roboteqState;
            }
        }

        public async Task UpdateThumbstickState(ThumbstickState thumbstickState)
        {
            if (thumbstickState.Timestamp > _LastThumstickState.Timestamp)
            {
                await Clients.All.ThumbstickStateUpdated(thumbstickState);
                _LastThumstickState = thumbstickState;
            }
        }
    }
}
