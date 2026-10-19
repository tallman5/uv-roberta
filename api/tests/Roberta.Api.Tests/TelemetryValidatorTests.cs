using Roberta.Api.Contracts;
using Roberta.Api.Services;

namespace Roberta.Api.Tests;

public sealed class TelemetryValidatorTests
{
    private static readonly DateTimeOffset Now = DateTimeOffset.Parse("2026-01-01T00:00:00Z");

    [Fact]
    public void AcceptsBoundedTelemetryWithGpsAndSeparateCameraUrl()
    {
        var telemetry = new RobertaTelemetry(
            1,
            "roberta-01",
            Now,
            "dashboard",
            true,
            false,
            null,
            new GpsFix(47.6, -122.3, Now, 2.5),
            [new CameraStream("front", "https://camera.example/stream")]);

        Assert.Null(TelemetryValidator.Validate(telemetry, "roberta-01", Now));
    }

    [Fact]
    public void RejectsMismatchedDeviceAndInvalidGps()
    {
        var telemetry = new RobertaTelemetry(
            1,
            "other-device",
            Now,
            "rc",
            false,
            false,
            null,
            new GpsFix(91, 0, Now, null),
            []);

        Assert.NotNull(TelemetryValidator.Validate(telemetry, "roberta-01", Now));

        var validDeviceInvalidGps = telemetry with
        {
            DeviceId = "roberta-01",
            Gps = new GpsFix(0, double.NaN, Now, null)
        };
        Assert.NotNull(TelemetryValidator.Validate(validDeviceInvalidGps, "roberta-01", Now));
    }

    [Fact]
    public void RejectsCredentialBearingCameraUrlsAndFutureTimestamps()
    {
        var credentialUrl = new RobertaTelemetry(
            1,
            "roberta-01",
            Now,
            "unknown",
            false,
            false,
            null,
            null,
            [new CameraStream("front", "https://user:password@camera.example/stream")]);
        Assert.NotNull(TelemetryValidator.Validate(credentialUrl, "roberta-01", Now));

        var futureTelemetry = credentialUrl with
        {
            CameraStreams = [],
            ObservedAt = Now.AddSeconds(2)
        };
        Assert.NotNull(TelemetryValidator.Validate(futureTelemetry, "roberta-01", Now));
    }
}
