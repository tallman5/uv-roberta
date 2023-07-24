using System.Net.NetworkInformation;
using System.Net;

namespace Roberta
{
    public class Utilities
    {
        public const string AUTH_SCHEMES = "AzureAD, AzureADOpenID, AzureADCookie, Bearer";

        private static readonly double p = Math.PI / 180; // 0.017453292519943295;

        public static double CalculateDistance(double lat1, double lon1, double lat2, double lon2)
        {
            var a = 0.5 - Math.Cos((lat2 - lat1) * p) / 2 +
                    Math.Cos(lat1 * p) * Math.Cos(lat2 * p) *
                    (1 - Math.Cos((lon2 - lon1) * p)) / 2;

            return 12742 * Math.Asin(Math.Sqrt(a)); // 2 * R; Earth Radius R = 6371 km
        }

        public static string GetLocalIPAddress()
        {
            try
            {
                // Get a list of all network interfaces on the machine
                NetworkInterface[] networkInterfaces = NetworkInterface.GetAllNetworkInterfaces();

                foreach (NetworkInterface networkInterface in networkInterfaces)
                {
                    // We only care about interfaces that are up and not loopback
                    if (networkInterface.OperationalStatus == OperationalStatus.Up &&
                        networkInterface.NetworkInterfaceType != NetworkInterfaceType.Loopback)
                    {
                        // Get all IP addresses assigned to this interface
                        IPInterfaceProperties properties = networkInterface.GetIPProperties();
                        foreach (UnicastIPAddressInformation ip in properties.UnicastAddresses)
                        {
                            // Check if the address is IPv4 and not the loopback address
                            if (ip.Address.AddressFamily == System.Net.Sockets.AddressFamily.InterNetwork &&
                                !IPAddress.IsLoopback(ip.Address))
                            {
                                return ip.Address.ToString();
                            }
                        }
                    }
                }

                // If no suitable IP address is found, return an empty string
                Console.WriteLine("Could not find an IP Address, using loopback");
                return IPAddress.Loopback.ToString();
            }
            catch (Exception ex)
            {
                Console.WriteLine("Error while getting IP address: " + ex.Message);
                return IPAddress.Loopback.ToString();
            }
        }

        public static string[] GetOrigins()
        {
            var returnValue = new string[]{
                "http://localhost:8000",
                "http://localhost:9000",
                "https://www.uvroberta.com",
                "https://agreeable-sky-00a48a010.3.azurestaticapps.net"
            };
            return returnValue;
        }

        public static int ScaleValue(int value)
        {
            return ScaleValue(value, short.MinValue, short.MaxValue, -1000, 1000);
        }

        public static int ScaleValue(int value, int minValue, int maxValue, int scaledMinValue, int scaledMaxValue)
        {
            int scaledValue = scaledMinValue + (int)((double)(value - minValue) / (maxValue - minValue) * (scaledMaxValue - scaledMinValue));
            return scaledValue;
        }
    }
}
