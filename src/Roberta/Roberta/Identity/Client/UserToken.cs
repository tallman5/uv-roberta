using Microsoft.Identity.Client;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Roberta.Identity.Client
{
    public class UserToken
    {
        public string AccessToken { get; set; }
        public DateTimeOffset Expires { get; set; }
        public List<string> Roles { get; set; }
        public string ScreenName { get; set; }
        public string UserName { get; set; }
        public UserToken()
        {
            AccessToken = ScreenName = UserName = string.Empty;
            Roles = new List<string>();
        }

        public static UserToken FromAuthResult(AuthenticationResult ar)
        {
            return new UserToken
            {
                AccessToken = ar.AccessToken,
                Expires = ar.ExpiresOn,
            };
        }
    }
}
