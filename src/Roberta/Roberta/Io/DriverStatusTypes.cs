namespace Roberta.Io
{
    internal enum KnownDriverStatusType
    {
        Waiting, Driving
    }

    [System.Runtime.Serialization.DataContract]
    public struct DriverStatusType
    {
        public static readonly DriverStatusType Empty = new(0, string.Empty, string.Empty);

        public override bool Equals(object? obj)
        {
            if (obj is DriverStatusType right)
            {
                return this == right;
            }
            return false;
        }

        public override int GetHashCode()
        {
            if (string.IsNullOrWhiteSpace(this.Name))
                return string.Empty.GetHashCode();
            else
                return this.Name.GetHashCode();
        }

        public static bool operator ==(DriverStatusType left, DriverStatusType right)
        {
            return (left.Name == right.Name);
        }

        public static bool operator !=(DriverStatusType left, DriverStatusType right)
        {
            return !(left == right);
        }

        public static DriverStatusType Parse(int id)
        {
            var returnValue = Parse(id.ToString());
            return returnValue;
        }

        public static DriverStatusType Parse(string s)
        {
            if (string.IsNullOrWhiteSpace(s)) throw new ArgumentException("Parameter s cannot be null or white space.", nameof(s));

            var returnValue = DriverStatusTypes.AllKnownDriverStatusTypes.Where(st => st.Name.ToLower() == s.ToLower()).FirstOrDefault();
            if (string.IsNullOrWhiteSpace(returnValue.Name))
                returnValue = DriverStatusTypes.AllKnownDriverStatusTypes.Where(st => st.Code.ToLower() == s.ToLower()).FirstOrDefault();
            if (string.IsNullOrWhiteSpace(returnValue.Name))
                returnValue = DriverStatusTypes.AllKnownDriverStatusTypes.Where(st => st.Id.ToString() == s).FirstOrDefault();
            if (string.IsNullOrWhiteSpace(returnValue.Name))
                throw new Exception(string.Format("A DriverStatusType with a name of '{0}' could not be found.", s));
            return returnValue;
        }

        internal DriverStatusType(int id, string name, string code) : this()
        {
            Id = id;
            Name = name;
            Code = code;
        }

        [System.Runtime.Serialization.DataMember]
        public int Id { get; internal set; }

        [System.Runtime.Serialization.DataMember]
        public string Name { get; internal set; }

        [System.Runtime.Serialization.DataMember]
        public string Code { get; internal set; }

        public override string ToString()
        {
            string returnValue = (string.IsNullOrWhiteSpace(this.Name)) ? "Empty" : this.Name;
            return returnValue;
        }

        public static bool TryParse(string s, out DriverStatusType result)
        {
            bool returnValue = true;
            result = DriverStatusType.Empty;
            try
            {
                if (!string.IsNullOrEmpty(s))
                    result = DriverStatusType.Parse(s);
            }
            catch { }
            if (result == DriverStatusType.Empty)
                returnValue = false;
            return returnValue;
        }
    }

    internal static class KnownDriverStatusTypes
    {
        private static readonly Dictionary<KnownDriverStatusType, DriverStatusType> _Cache;

        internal static DriverStatusType FromKnown(KnownDriverStatusType knownDriverStatusType)
        {
            return KnownDriverStatusTypes._Cache.Where(p => p.Key == knownDriverStatusType).FirstOrDefault().Value;
        }

        static KnownDriverStatusTypes()
        {
            KnownDriverStatusTypes._Cache = new Dictionary<KnownDriverStatusType, DriverStatusType>
            {
                { KnownDriverStatusType.Waiting, new DriverStatusType(1, "Waiting to Drive", "Waiting") },
                { KnownDriverStatusType.Driving, new DriverStatusType(2, "Driving", "Driving") }
            };

        }

        internal static void LoadKnown(List<DriverStatusType> list)
        {
            if (null == list) throw new ArgumentNullException(nameof(list));
            list.Clear();
            foreach (var p in KnownDriverStatusTypes._Cache)
                list.Add(p.Value);
        }
    }

    public sealed class DriverStatusTypes
    {
        public static List<DriverStatusType> AllKnownDriverStatusTypes
        {
            get
            {
                var returnValue = new List<DriverStatusType>();
                KnownDriverStatusTypes.LoadKnown(returnValue);
                return returnValue;
            }
        }

        public static List<DriverStatusType> OrderByCode
        {
            get
            {
                var returnValue = new List<DriverStatusType>();
                KnownDriverStatusTypes.LoadKnown(returnValue);
                return returnValue.OrderBy(st => st.Code).ToList();
            }
        }

        public static List<DriverStatusType> OrderByName
        {
            get
            {
                var returnValue = new List<DriverStatusType>();
                KnownDriverStatusTypes.LoadKnown(returnValue);
                return returnValue.OrderBy(st => st.Name).ToList();
            }
        }

        public static DriverStatusType Waiting { get { return KnownDriverStatusTypes.FromKnown(KnownDriverStatusType.Waiting); } }
        public static DriverStatusType Driving { get { return KnownDriverStatusTypes.FromKnown(KnownDriverStatusType.Driving); } }

    }

    // Do not delete these comments, used if struct needs changes
    //Waiting to Drive,Waiting,Waiting,1
    //Driving,Driving,Driving,2
}
