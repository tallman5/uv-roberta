using Roberta.Api.Contracts;
using Roberta.Api.Services;

namespace Roberta.Api.Tests;

public sealed class CommandValidatorTests
{
    private static readonly DateTimeOffset Now = DateTimeOffset.Parse("2026-01-01T00:00:00Z");

    [Fact]
    public void AcceptsBoundedCurrentCommand()
    {
        var command = new DriveCommand(1, Guid.NewGuid(), -1, 1, Now.AddMilliseconds(500));

        Assert.Null(CommandValidator.Validate(command, Now));
    }

    [Theory]
    [InlineData(double.NaN, 0)]
    [InlineData(double.PositiveInfinity, 0)]
    [InlineData(-1.01, 0)]
    [InlineData(0, 1.01)]
    public void RejectsNonFiniteOrOutOfRangeDrive(double throttle, double steering)
    {
        var command = new DriveCommand(1, Guid.NewGuid(), throttle, steering, Now.AddMilliseconds(500));

        Assert.NotNull(CommandValidator.Validate(command, Now));
    }

    [Fact]
    public void RejectsWrongVersionAndEmptyId()
    {
        Assert.NotNull(CommandValidator.Validate(
            new DriveCommand(2, Guid.NewGuid(), 0, 0, Now.AddMilliseconds(500)), Now));
        Assert.NotNull(CommandValidator.Validate(
            new DriveCommand(1, Guid.Empty, 0, 0, Now.AddMilliseconds(500)), Now));
    }

    [Fact]
    public void RejectsExpiredOrExcessivelyLongCommand()
    {
        Assert.NotNull(CommandValidator.Validate(
            new DriveCommand(1, Guid.NewGuid(), 0, 0, Now), Now));
        Assert.NotNull(CommandValidator.Validate(
            new DriveCommand(1, Guid.NewGuid(), 0, 0, Now.AddSeconds(1).AddTicks(1)), Now));
    }
}
