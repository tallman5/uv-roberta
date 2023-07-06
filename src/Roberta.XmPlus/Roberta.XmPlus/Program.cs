using System.Device.Gpio;

Console.WriteLine("Listening, press Enter to exit...");

int[] buffer = new int[350];
int pin = 17;
int bitIndex = 0;
var start = DateTimeOffset.Now;

using GpioController gpioController = new();
gpioController.OpenPin(pin, PinMode.Input);
gpioController.RegisterCallbackForPinValueChangedEvent(pin, PinEventTypes.Rising | PinEventTypes.Falling, Pin_Changed);

Console.ReadLine();
gpioController.ClosePin(pin);
gpioController.Dispose();
Console.WriteLine();


void Pin_Changed(object sender, PinValueChangedEventArgs e)
{
    int pulseWidth = (int)Math.Round(((DateTimeOffset.Now - start).TotalMilliseconds) * 100);

    //if (!gpioController.IsPinOpen(pin)) return;
    int readVal = (e.ChangeType == PinEventTypes.Falling) ? 0 : 1;
    int newVal = (-readVal + 1);

    if (pulseWidth > 500)
    {
        Console.Write($"  {bitIndex}");
        Console.WriteLine();
        bitIndex = 0;
    }
    else
    {
        for (int i = 0; i < pulseWidth; i++)
        {
            Console.Write(readVal);
            bitIndex++;
        }
    }

    start = DateTimeOffset.Now;
}
