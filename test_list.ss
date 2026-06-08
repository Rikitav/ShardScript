namespace Test
{
	public class Program
	{
		public static fn Main() -> void
		{
			var list = new System.Collections.List<int>();
			list.Add(10);
			list.Add(20);
			list.Add(30);

			println(list.Length);
			println(list.ElementAt(0));
			println(list.ElementAt(1));
			println(list.ElementAt(2));

			list.RemoveAt(1);
			println(list.Length);
			println(list.ElementAt(0));
			println(list.ElementAt(1));

			list.Clear();
			println(list.Length);
		}
	}
}
