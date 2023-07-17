using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.SignalR;
using Roberta.Hub.Hubs;
using Roberta.Io;

namespace Roberta.Hub.Controllers
{
    public class RobertaController : BaseController
    {
        private readonly IHubContext<RobertaHub, IRobertaClient> _hubContext;
        private DateTimeOffset _lastRxTimestamp;
        private RobertaHub _robertaHub;

        public RobertaController(ILogger<RobertaController> logger, IConfiguration configuration, IHubContext<RobertaHub, IRobertaClient> hubContext)
            : base(logger, configuration)
        {
            _hubContext = hubContext;
            _lastRxTimestamp = DateTimeOffset.MinValue;
            _robertaHub = _hubContext.Clients.Groups("gn") as RobertaHub;
        }

        [HttpGet("getTest")]
        public IActionResult GetTest()
        {
            return Ok();
        }

        [HttpPost("udpateRxState")]
        public IActionResult UpdateRxState([FromBody] RxState rxState)
        {

            try
            {
                if (rxState.ChannelValues.Count != 16)
                    return BadRequest("ChannelValues must contain exactly 16 items.");
                if (rxState.Timestamp > _lastRxTimestamp)
                {
                    _hubContext.Clients.All.RxStateUpdated(rxState);
                    _lastRxTimestamp = rxState.Timestamp;
                }
                return Ok();
            }
            catch (Exception ex)
            {
                _logger?.LogError(ex, null);
                return StatusCode(StatusCodes.Status500InternalServerError);
            }
        }
    }
}
