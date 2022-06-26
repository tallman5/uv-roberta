using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.ComponentModel.DataAnnotations;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;

namespace Roberta.ComponentModel
{
    public interface IItem
    {
        Guid Id { get; set; }
        string Title { get; set; }
    }

    public class Item : NotifyPropertyChanged, IItem
    {

        private Guid _Id;
        [Key]
        [DataMember]
        public Guid Id
        {
            get { return this._Id; }
            set
            {
                if (value != this._Id)
                {
                    this._Id = value;
                    this.RaisePropertyChanged(nameof(Id));
                }
            }
        }

        public Item() : this(Guid.NewGuid(), string.Empty) { }

        public Item(Guid id, string title)
        {
            this._Id = id;
            this._Title = title;
        }

        private string _Title;
        [DataMember]
        public virtual string Title
        {
            get { return this._Title; }
            set
            {
                if (value != this._Title)
                {
                    this._Title = value;
                    this.RaisePropertyChanged(nameof(Title));
                }
            }
        }
    }
}
