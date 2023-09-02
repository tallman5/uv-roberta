using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.SignalR;
using Roberta.Hub.Services;
using Roberta.Io;
using System.Collections.ObjectModel;

namespace Roberta.Hub.Hubs
{
    [Authorize]
    public class RobertaHub : Hub<IRobertaClient>
    {
        // TODO: remove static _Drivers when app goes distributed
        // TODO: Ensure SetXY is coming from current driver

        const string RobertaGroup = "RobertaGroup";
        private static ObservableCollection<Driver> _Drivers;
        private static GpsState _LastGpsState;
        private static RoboteqState _LastRoboteqState;
        private static ThumbstickState _LastThumstickState;
        private static int _GuestIndex = 1;

        public const string GpsStateUpdated = "GpsStateUpdated";
        public const string RoboteqStateUpdated = "RoboteqStateUpdated";

        private readonly RoboteqBackgroundService _roboteqBackgroundService;

        public RobertaHub(RoboteqBackgroundService roboteqBackgroundService)
        {
            _roboteqBackgroundService = roboteqBackgroundService;

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
            if (Context.User.IsInRole("roberta.devices"))
            {
                Groups.AddToGroupAsync(Context.ConnectionId, RobertaGroup);
            }
            else
            {
                var userName = Context.User.Identity.Name;
                if (string.IsNullOrWhiteSpace(userName))
                    userName = "guest@mcgurkin.net";

                var existingDriver = _Drivers.Where(d => d.ConnectionId == Context.ConnectionId).FirstOrDefault();
                if (null == existingDriver)
                {
                    var newDriver = new Driver
                    {
                        ConnectionId = Context.ConnectionId,
                        DriverStatusType = DriverStatusTypes.Waiting,
                        Title = "Driver",
                        UserName = userName
                    };

                    switch (newDriver.UserName)
                    {
                        case "joseph@mcgurkin.net":
                            newDriver.ScreenName = "tallman";
                            _Drivers.Insert(0, newDriver);
                            break;
                        case "guest@mcgurkin.net":
                            newDriver.ScreenName = $"guest {_GuestIndex}";
                            _GuestIndex++;
                            _Drivers.Add(newDriver);
                            break;
                        default:
                            newDriver.ScreenName = newDriver.UserName;
                            _Drivers.Add(newDriver);
                            break;
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
                Clients.All?.DriversUpdated(_Drivers);
            }

            if (_Drivers.Count < 1)
            {
                _roboteqBackgroundService.SetXY(0, 0);
            }

            if (Context.User.IsInRole("roberta.devices"))
            {
                _LastGpsState.IsReady = false;
                _LastRoboteqState.IsReady = false;

                Clients.Caller.GpsStateUpdated(_LastGpsState);
                Clients.Caller.RoboteqStateUpdated(_LastRoboteqState);
            }
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

        [Authorize(Roles = "roberta.admins")]
        public async Task SetPowerScale(decimal powerScale)
        {
            await Clients.Group(RobertaGroup).SetPowerScale(powerScale);
            _roboteqBackgroundService.SetPowerScale(powerScale);
        }

        public async Task SetXY(decimal x, decimal y)
        {
            await Clients.Group(RobertaGroup).SetXY(x, y);
            _roboteqBackgroundService.SetXY(x, y);
        }

        [Authorize(Roles = "roberta.admins")]
        public async Task StartDriving(string connectionId)
        {
            await StopDriving();

            var newDriver = _Drivers.Where(d => d.ConnectionId == connectionId).FirstOrDefault();
            if (null != newDriver)
            {
                newDriver.DriverStatusType = DriverStatusTypes.Driving;
                await Clients.All.DriversUpdated(_Drivers);
            }
        }

        [Authorize(Roles = "roberta.admins")]
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

        [Authorize(Roles = "roberta.devices")]
        public async Task UpdateRoboteqState(RoboteqState roboteqState)
        {
            if (roboteqState.Timestamp > _LastRoboteqState.Timestamp)
            {
                await Clients.AllExcept(Context.ConnectionId).RoboteqStateUpdated(roboteqState);
                _LastRoboteqState = roboteqState;
            }
        }

        [Authorize(Roles = "roberta.devices")]
        public async Task UpdateThumbstickState(ThumbstickState thumbstickState)
        {
            if (thumbstickState.Timestamp > _LastThumstickState.Timestamp)
            {
                await Clients.AllExcept(Context.ConnectionId).ThumbstickStateUpdated(thumbstickState);
                _LastThumstickState = thumbstickState;
            }
        }
    }
}
