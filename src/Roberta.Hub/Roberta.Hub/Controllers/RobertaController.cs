using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Http;
using Microsoft.AspNetCore.Mvc;

namespace Roberta.Hub.Controllers
{
    [AllowAnonymous]
    [Route("api/[controller]")]
    [ApiController]
    public class RobertaController : BaseController
    {
        public RobertaController(ILogger<RobertaController> logger, IConfiguration configuration)
            : base(logger, configuration)
        {
        }
    }
}
