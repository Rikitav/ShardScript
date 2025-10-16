using System.IO;

namespace Shard
{
	public class Script
	{
		public static void Main()
		{
			print(GetMsg());
		}

		static int GetMsg()
		{
			return "Hello, World!";
		}
	}
}
