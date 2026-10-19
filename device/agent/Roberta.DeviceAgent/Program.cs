using Roberta.DeviceAgent;

var builder = Host.CreateApplicationBuilder(args);
builder.Services.AddSingleton<ILocalControl, UnconfiguredLocalControl>();
builder.Services.AddHostedService<Worker>();

var host = builder.Build();
host.Run();
