using Microsoft.AspNetCore.Components;
using Microsoft.AspNetCore.SignalR.Client;
using Microsoft.JSInterop;

namespace Roberta.Web.Shared
{
    public class BaseComponent : ComponentBase
    {
        [Inject]
        protected IConfiguration? Configuration { get; set; }

        private HttpClientHandler? handler;

        protected HubConnection? HubConnection { get; set; }

        [Inject]
        protected IJSRuntime? JSRuntime { get; set; }

        [Inject]
        protected NavigationManager? NavigationManager { get; set; }

        protected override async Task OnInitializedAsync()
        {
            await base.OnInitializedAsync();

            handler = new HttpClientHandler
            {
#if DEBUG
                // Ignore certificate validation errors
                ServerCertificateCustomValidationCallback = (sender, cert, chain, sslPolicyErrors) => true
#endif
            };

            HubConnection = new HubConnectionBuilder()
                .WithUrl(NavigationManager!.ToAbsoluteUri("/robertaHub"), options =>
                {
                    options.HttpMessageHandlerFactory = _ => handler;
                })
                .WithAutomaticReconnect()
                .Build();
        }
    }
}
