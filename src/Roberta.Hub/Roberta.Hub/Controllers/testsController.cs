using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;

namespace Roberta.Hub.Controllers
{
    [Authorize]
    [Route("api/[controller]")]
    [ApiController]
    public class testsController : BaseController
    {
        public testsController(ILogger<testsController> logger, IConfiguration configuration)
            : base(logger, configuration) { }

        [HttpPost("echo")]
        [AllowAnonymous]
        public Msg Echo(Msg message)
        {
            message.M += "...processed";
            return message;
        }

        [Authorize(AuthenticationSchemes = "AllSchemes")]
        [HttpGet("getBad")]
        public DateTime GetBad()
        {
            return DateTime.Now;
        }

        [HttpGet("hw")]
        [AllowAnonymous]
        public string Hw()
        {
            return "Hellow World";
        }

        [HttpGet("hw-auth")]
        [Authorize(AuthenticationSchemes = Utilities.AUTH_SCHEMES)]
        public string HwAuth()
        {
            return "Hellow World";
        }

        [HttpGet("hw-auth-admin")]
        [Authorize(Roles = "roberta.admins")]
        public string HwAuthAdmin()
        {
            return "Hellow World";
        }
    }

    public class Msg
    {
        public Msg() { M = "Initialized"; }
        public string M { get; set; }
    }
}
