// ASCII C#.net6.0 TAB4 CRLF
// Dependens: None
// LastCheck: 20240406
// AllAuthor: @dosconio
// DocuTitle: {TEMPLATE}
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Unisym
{
	public class _Hello
	{
		public string Name { get; set; }
		public string Description { get; set; }
		public _Hello(string Name = "Alice", string Description = "dosconio")
		{
			this.Name = Name;
			this.Description = Description;
			Console.WriteLine("Ciallo, minasan~");
		}

	}
}
