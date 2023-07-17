using Microsoft.AspNetCore.Mvc;

namespace Roberta.Hub.Controllers
{
    public class BaseController : ControllerBase
    {
        protected readonly IConfiguration _configuration;
        protected readonly ILogger _logger;

        public BaseController(ILogger logger, IConfiguration configuration)
        {
            _configuration = configuration;
            _logger = logger;
        }
    }
}
