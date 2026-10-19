using Microsoft.AspNetCore.Authentication.JwtBearer;
using Roberta.Api.Hubs;
using Roberta.Api.Services;

var builder = WebApplication.CreateBuilder(args);

var authority = builder.Configuration["Authentication:Authority"];
var audience = builder.Configuration["Authentication:Audience"];
if (string.IsNullOrWhiteSpace(authority) || string.IsNullOrWhiteSpace(audience))
{
    throw new InvalidOperationException(
        "Authentication:Authority and Authentication:Audience must be configured before the API starts.");
}

var allowedOrigins = builder.Configuration.GetSection("Cors:AllowedOrigins").Get<string[]>() ?? [];
if (allowedOrigins.Length == 0 || allowedOrigins.Any(string.IsNullOrWhiteSpace))
{
    throw new InvalidOperationException(
        "Configure at least one exact dashboard origin in Cors:AllowedOrigins.");
}

builder.Services
    .AddAuthentication(JwtBearerDefaults.AuthenticationScheme)
    .AddJwtBearer(options =>
    {
        options.Authority = authority;
        options.Audience = audience;
        options.RequireHttpsMetadata = !builder.Environment.IsDevelopment();
        options.TokenValidationParameters.RoleClaimType = "role";
        options.TokenValidationParameters.NameClaimType = "sub";
        options.Events = new JwtBearerEvents
        {
            OnMessageReceived = context =>
            {
                var token = context.Request.Query["access_token"];
                if (!string.IsNullOrEmpty(token) &&
                    context.HttpContext.Request.Path.StartsWithSegments("/hubs/roberta"))
                {
                    context.Token = token;
                }

                return Task.CompletedTask;
            }
        };
    });

builder.Services.AddCors(options =>
{
    options.AddPolicy("dashboard", policy =>
        policy.WithOrigins(allowedOrigins).AllowAnyHeader().AllowAnyMethod().AllowCredentials());
});
builder.Services.AddAuthorization();
builder.Services.AddSignalR(options =>
{
    options.MaximumReceiveMessageSize = 16 * 1024;
    options.EnableDetailedErrors = builder.Environment.IsDevelopment();
});
builder.Services.AddSingleton<DeviceConnectionRegistry>();
builder.Services.AddSingleton<PendingCommandRegistry>();

var app = builder.Build();

app.UseCors("dashboard");
app.UseAuthentication();
app.UseAuthorization();
app.MapGet("/healthz", () => Results.Ok(new { status = "ok" }));
app.MapHub<RobertaHub>("/hubs/roberta");

app.Run();

public partial class Program;
