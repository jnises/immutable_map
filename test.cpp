#include <iostream>
#include <cassert>
#include <atomic>

#include "immutable_map.hpp"

namespace deepness
{
void test0()
{
    immutable_map<int, const int> intmap;
    auto intmap2 = intmap.set(0, 42);
    assert(intmap2[0] == 42);
    bool exception = false;
    try
    {
        intmap[0];
    }
    catch(key_error &)
    {
        exception = true;
    }
    assert(exception);

    auto intmap3 = intmap2.set(0, 1);
    assert(intmap3[0] == 1);
}

void test1()
{
    immutable_map<int, const int> intmap;
    intmap = intmap.set(0, 1);
    assert(intmap[0] == 1);
    intmap = intmap.erase(0);
    bool exception = false;
    try
    {
        intmap[0];
    }
    catch(key_error &)
    {
        exception = true;
    }
    assert(exception);
}

void test2()
{
    immutable_map<int, const int> intmap;
    for(size_t i = 0; i < 10000; ++i)
    {
        intmap = intmap.set(i, i);
		assert(intmap[i] == i);
    }

    for(size_t i = 0; i < 10000; ++i)
    {
        assert(intmap[i] == i);
    }
}

void test3()
{
	immutable_map<int, int> intmap;
	intmap = intmap.set(0, 0);
	assert(intmap[0] == 0);
	// since we return a non-const reference this is possible
	intmap[0] = 4;
	assert(intmap[0] == 4);
}

class testhash
{
public:
    typedef int argument_type;
    typedef size_t result_type;

    result_type operator()(argument_type arg)
    {
        return 0;
    }
};

/**
 * Make sure that the map works with hash collisions.
 */
void test4()
{
    immutable_map<int, int, testhash> intmap;
    intmap = intmap.set(0, 0);
    assert(intmap[0] == 0);
    intmap = intmap.set(1, 1);
    assert(intmap[1] == 1);
}

void testAtomic()
{
	using namespace std;
	atomic<immutable_map<string, string>> strmap;
	strmap = strmap.load().set("a", "b");
	auto nonatomic = strmap.load();
	auto result = nonatomic["a"];
	assert(result == "b");
}
}

int main(int argc, char *argv[])
{
    using namespace deepness;
    std::cout << "immutable_map tests" << std::endl;

    test0();
    test1();
    test2();
    test3();
    test4();
	//testAtomic();

    std::cout << "tests complete" << std::endl;
}
