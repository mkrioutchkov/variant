#include "dependency_injector.h"

int main()
{
	struct test_type_0
	{
		test_type_0(int i) : i(i) {}

		int i = 0;
	};

	mdk::dependency_injector injector;
	auto& i = injector.try_emplace<int>(1);
	std::cout << i << std::endl;
	i = 2;
	std::cout << *injector.force_get<int>() << std::endl;

	auto& q = injector.try_emplace<test_type_0>(22);
	std::cout << q.i << std::endl;

	auto q2 = injector.get< test_type_0>();
	std::cout << q2->i << std::endl;

	struct test_type_1
	{
		test_type_1(std::string s)
		{
			std::cout << "test_type_1: " << s.c_str() << std::endl;
		}
	};

	auto tuple = std::make_tuple(std::string("my string"));
	injector.lazy_emplace<test_type_1>(std::move(tuple));
	auto q3 = injector.get<test_type_1>(); 	// only at this point does test_type_1 get created
	std::cout << "q3 address: " << q3;
}
