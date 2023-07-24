using Microsoft.AspNetCore.Authentication.JwtBearer;
using Microsoft.Extensions.Hosting;
using Microsoft.Identity.Web;
using Roberta;
using Roberta.Hub.Hubs;
using Swashbuckle.AspNetCore.SwaggerUI;
using System.Net.Http;

var builder = WebApplication.CreateBuilder(args);

// When running on a Pi, issues loading appsettings.json
// Just spit out one of the settings to ensure loaded
var configVal = builder.Configuration.GetSection("AzureAd:Domain").Value;
var msg = $"==== Domain Config Val: {configVal} ====";
Console.WriteLine(msg);

#if !DEBUG
builder.Configuration.AddJsonFile("appsettings.Release.json");
builder.WebHost.ConfigureKestrel((context, serverOptions) =>
{
    var kestrelSection = context.Configuration.GetSection("Kestrel");

    serverOptions.Configure(kestrelSection)
        .Endpoint("HTTPS", listenOptions =>
        {
            // ...
        });
});
#endif

builder.Services.AddAuthentication(JwtBearerDefaults.AuthenticationScheme)
    .AddMicrosoftIdentityWebApi(builder.Configuration.GetSection("AzureAd"))
        .EnableTokenAcquisitionToCallDownstreamApi()
            .AddMicrosoftGraph(builder.Configuration.GetSection("MicrosoftGraph"))
            .AddInMemoryTokenCaches();

builder.Services.AddSignalR();
builder.Services.AddControllers();
builder.Services.AddEndpointsApiExplorer();
builder.Services.AddSwaggerGen();

var app = builder.Build();

app.UseStaticFiles();
app.UseSwagger();
app.UseSwaggerUI(c =>
{
    c.SwaggerEndpoint("/swagger/v1/swagger.json", "Roberta API v1");
    c.DocExpansion(DocExpansion.None);
});
app.UseHttpsRedirection();
app.UseAuthentication();
app.UseAuthorization();
app.MapControllers();
app.MapHub<RobertaHub>("/robertaHub");
app.UseCors(builder =>
{
    builder
        .AllowAnyMethod()
        .AllowAnyHeader()
        .AllowCredentials()
        .WithOrigins(Utilities.GetOrigins());
});

app.Run();
