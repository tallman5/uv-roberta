using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Http;
using Microsoft.AspNetCore.Mvc;
using Roberta.Identity.Client;

namespace Roberta.Hub.Controllers
{
    [Route("api/[controller]")]
    [ApiController]
    public class userController : BaseController
    {
        public userController(ILogger<testsController> logger, IConfiguration configuration)
            : base(logger, configuration) { }

        [HttpGet("guestToken")]
        [AllowAnonymous]
        public UserToken GetGuestToken()
        {
            string tenantId = _configuration["AzureAd:TenantId"];
            string clientId = _configuration["WebClientId"];
            string scope = _configuration["Scope"];
            string clientSecret = _configuration["WebSecret"];
            var ar = Utilities.GetClientToken(tenantId, clientId, clientSecret, scope);
            var returnValue = UserToken.FromAuthResult(ar);
            return returnValue;
        }
    }
}
