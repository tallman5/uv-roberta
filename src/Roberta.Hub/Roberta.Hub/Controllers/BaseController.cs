using Microsoft.AspNetCore.Mvc;

namespace Roberta.Hub.Controllers
{
    public class BaseController : ControllerBase
    {
        protected readonly IConfiguration _configuration;
        protected readonly ILogger _logger;
        protected readonly string ValidSchemes = "Bearer";

        public BaseController(ILogger logger, IConfiguration configuration)
        {
            _configuration = configuration;
            _logger = logger;
        }
    }
}
