using Microsoft.AspNetCore.SignalR;
using Roberta.Hub.Hubs;
using System.IO.Ports;

namespace Roberta.Hub.Services
{
    public class BaseBackgroundService : BackgroundService
    {
        protected readonly IConfiguration _configuration;
        protected readonly IHubContext<RobertaHub> _hubContext;
        protected readonly ILogger _logger;

        public BaseBackgroundService(IHubContext<RobertaHub> hubContext, ILogger logger, IConfiguration configuration)
        {
            _configuration = configuration;
            _hubContext = hubContext;
            _logger = logger;
        }

        protected override Task ExecuteAsync(CancellationToken stoppingToken)
        {
            throw new NotImplementedException();
        }
    }
}
