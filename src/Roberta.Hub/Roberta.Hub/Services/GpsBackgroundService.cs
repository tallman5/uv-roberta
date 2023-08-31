using Microsoft.AspNetCore.SignalR;
using Roberta.Hub.Hubs;
using Roberta.Io;
using System.IO.Ports;

namespace Roberta.Hub.Services
{
    public class GpsBackgroundService : BaseBackgroundService
    {
        private readonly SerialPort _SerialPort;
        private GpsState _state;
        private readonly string _portName;

        public GpsBackgroundService(IHubContext<RobertaHub> hubContext, ILogger<GpsBackgroundService> logger, IConfiguration configuration)
            : base(hubContext, logger, configuration)
        {
            _portName = configuration["Roberta:GpsPort"];
            _state = new GpsState { Title = "GPS" };
            _SerialPort = new SerialPort
            {
                PortName = _portName,
                BaudRate = 4800,
                Parity = Parity.None,
                DataBits = 8,
                StopBits = StopBits.One
            };
            _SerialPort.DataReceived += SerialPort_DataReceived;
        }

        protected override async Task ExecuteAsync(CancellationToken stoppingToken)
        {
            while (!stoppingToken.IsCancellationRequested)
            {
                if (!_SerialPort.IsOpen)
                {
                    try
                    {
                        _SerialPort.Open();
                        Console.WriteLine($"GPS connected on port {_portName}");
                    }
                    catch
                    {
                        Console.WriteLine($"Could not connect to GPS on port {_portName}, retrying in 10 seconds...");
                    }
                }
                await Task.Delay(TimeSpan.FromSeconds(10), stoppingToken);
            }

            if (null != _SerialPort)
            {
                if (_SerialPort.IsOpen) _SerialPort.Close();
                _SerialPort.Dispose();
            }
        }

        private void SerialPort_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            string newLine;

            try
            {
                if (!_SerialPort.IsOpen) return;
                newLine = _SerialPort.ReadLine();
                _SerialPort.DiscardInBuffer();
            }
            catch
            {
                return;
            }

            if (string.IsNullOrWhiteSpace(newLine)) return;
            bool isValid = false;
            string sentenceToCheck = "";

            try
            {
                sentenceToCheck = newLine.Trim();
                int checksum = Convert.ToByte(sentenceToCheck[sentenceToCheck.IndexOf('$') + 1]);
                for (int i = sentenceToCheck.IndexOf('$') + 2; i < sentenceToCheck.IndexOf('*'); i++)
                    checksum ^= Convert.ToByte(sentenceToCheck[i]);
                var sentenceChecksum = sentenceToCheck.Split(new char[] { '*' }, StringSplitOptions.RemoveEmptyEntries);
                isValid = sentenceChecksum[1] == checksum.ToString("X2");
            }
            catch { }

            if (!isValid) return;

            var fields = sentenceToCheck.Split(new char[] { ',' });

            switch (fields[0])
            {
                case "$GPRMC":
                    double lat, lon, speed, heading;
                    lat = GpsConnection.GetDegrees(fields[3], fields[4]);
                    lon = GpsConnection.GetDegrees(fields[5], fields[6]);
                    double.TryParse(fields[7], out speed);
                    double.TryParse(fields[8], out heading);
                    _state.SetVals(lat, lon, speed, heading);
                    _hubContext.Clients.All.SendAsync(RobertaHub.GpsStateUpdated, _state);
                    break;
            }
        }
    }
}
