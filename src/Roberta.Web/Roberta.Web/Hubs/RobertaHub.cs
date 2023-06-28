using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.SignalR;
using Roberta.Io;
using System.Collections.ObjectModel;

namespace Roberta.Web.Hubs
{
    public class RobertaHub : Hub<IRobertaClient>
    {
        private int streamCount;

        public RobertaHub()
        {
            streamCount = 0;
        }

        public async Task StartStream()
        {
            // Code to start the webcam stream
            // Broadcast frames to clients using "Clients.All.SendAsync()"
            streamCount++;

            using HttpClient httpClient = new HttpClient();

            while (true)
            {
                byte[] frameData;
                try
                {
                    HttpResponseMessage response = await httpClient.GetAsync($"http://rofo.mcgurkin.net:9080/snapshot");
                    if (response.IsSuccessStatusCode)
                    {
                        frameData = await response.Content.ReadAsByteArrayAsync();
                        // Broadcast the frame to connected clients
                        await Clients.All.FrameUpdated(frameData);
                    }
                }
                catch (Exception ex)
                {
                    // Handle any errors that occur during the stream reading process
                    Console.WriteLine($"Error reading stream: {ex.Message}");
                }

                // Delay between each frame retrieval (based on desired frequency)
                await Task.Delay(TimeSpan.FromSeconds(1));
            }
        }

        public void StopStream()
        {
            streamCount--;
        }

        public async Task UpdateGpsMixerState(GpsMixerState gpsMixerState)
        {
            await Clients.All.GpsMixerStateUpdated(gpsMixerState);
        }

        //public Task GpsMixerStateUpdated(GpsMixerState gpsMixerState)
        //{
        //    Clients.All.GpsMixerStateUpdated(gpsMixerState);
        //}

        public Task AddDriver(string userName)
        {
            throw new NotImplementedException();
        }

        public Task DriversUpdated(ObservableCollection<Driver> drivers)
        {
            throw new NotImplementedException();
        }

        public Task GpsStateUpdated(GpsState gpsState)
        {
            throw new NotImplementedException();
        }

        public Task Iam(string screenName)
        {
            throw new NotImplementedException();
        }

        public Task MessageReceived(DateTimeOffset messageDate, string message)
        {
            throw new NotImplementedException();
        }

        public Task RemoveDriver(string userName)
        {
            throw new NotImplementedException();
        }

        public Task RoboteqStateUpdated(RoboteqState roboteqState)
        {
            throw new NotImplementedException();
        }

        public Task SetCommandPriority(CommandPriority commandPriority)
        {
            throw new NotImplementedException();
        }

        public Task SetDriver(string userName)
        {
            throw new NotImplementedException();
        }

        public Task SetPowerScale(decimal powerScale)
        {
            throw new NotImplementedException();
        }

        public Task SetXY(decimal x, decimal y)
        {
            throw new NotImplementedException();
        }

        public Task StartDriving(string connectionId)
        {
            throw new NotImplementedException();
        }

        public Task StopDriving()
        {
            throw new NotImplementedException();
        }
    }
}
