#include "dependency_injector.h"

int main()
{
	struct non_copy_or_moveable
	{
		non_copy_or_moveable(int i) : i(i) {}
		non_copy_or_moveable(const non_copy_or_moveable&) = delete;

		int i = 0;
	};

	mdk::dependency_injector injector;
	auto& i = injector.emplace<int>(1);
	std::cout << i << std::endl;
	i = 2;
	std::cout << *injector.force_get<int>() << std::endl;

	auto& q = injector.emplace<non_copy_or_moveable>(22);
	std::cout << q.i << std::endl;
}
