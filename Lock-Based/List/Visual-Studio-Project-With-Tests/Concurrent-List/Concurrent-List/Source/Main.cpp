#include "ConcurrentList.h"

bool p(int i)
{
	return i == 1;
}

int main(int argc, char* argv[])
{
	ConcurrentList<int> list;

	for (int i = 1; i < 4; ++i)
	{
		list.PushToFront(i);
	}

	list.ForEach([&](int& integer) -> void { integer*=2; });

	list.FindFirstIf([&](int& integer) -> bool { return integer == 2; });

	list.RemoveIf([&](int& integer) -> bool { return integer % 2 == 0; });

	return 0;
}