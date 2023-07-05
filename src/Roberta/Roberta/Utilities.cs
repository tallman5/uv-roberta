namespace Roberta
{
    public class Utilities
    {
        private static readonly double p = 0.017453292519943295;    // Math.PI / 180

        public static double CalculateDistance(double lat1, double lon1, double lat2, double lon2)
        {
            var a = 0.5 - Math.Cos((lat2 - lat1) * p) / 2 +
                    Math.Cos(lat1 * p) * Math.Cos(lat2 * p) *
                    (1 - Math.Cos((lon2 - lon1) * p)) / 2;

            return 12742 * Math.Asin(Math.Sqrt(a)); // 2 * R; Earth Radius R = 6371 km
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
