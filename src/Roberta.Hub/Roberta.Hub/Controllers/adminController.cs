using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;

namespace Roberta.Hub.Controllers
{
    [Authorize]
    [Route("api/[controller]")]
    [ApiController]
    public class adminController : BaseController
    {
        public adminController(ILogger<adminController> logger, IConfiguration configuration)
            : base(logger, configuration) { }

        [HttpPost("echo")]
        [AllowAnonymous]
        public Msg Echo(Msg message)
        {
            message.M += "...processed";
            return message;
        }

        [HttpGet("hw")]
        [AllowAnonymous]
        public string Hw()
        {
            return "Hellow World";
        }

        [HttpGet("hw-auth")]
        [Authorize]
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
