using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Roberta.Io
{
    public interface IRobertaClient
    {
        Task Iam(string screenName);
        Task MessageReceived(DateTimeOffset messageDate, string message);

        // Drivers
        Task AddDriver(string userName);
        Task DriversUpdated(ObservableCollection<Driver> drivers);
        Task SetDriver(string userName);
        Task StartDriving(string connectionId);
        Task StopDriving();
        Task RemoveDriver(string userName);

        // Driving
        Task SetCommandPriority(CommandPriority commandPriority);
        Task SetXY(decimal x, decimal y);
        Task SetPowerScale(decimal powerScale);

        // Roberta
        Task GpsMixerStateUpdated(GpsMixerState gpsMixerState);
        Task GpsStateUpdated(GpsState gpsState);
        Task RoboteqStateUpdated(RoboteqState roboteqState);

        // Webcam Stream Relay
        Task FrameUpdated(byte[] frameData);
    }
}
