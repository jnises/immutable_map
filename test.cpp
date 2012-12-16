#include <iostream>
#include <cassert>

#include "immutable_map.hpp"

using namespace deepness;

int main(int argc, char *argv[])
{
    std::cout << "immutable_map tests" << std::endl;
    immutable_map<int, const int> intmap;
    auto intmap2 = intmap.set(0, 42);
    assert(intmap2[0] == 42);

    auto intmap3 = intmap2.set(0, 1);
    assert(intmap3[0] == 1);

    intmap3 = intmap2.erase(0);
    bool exception = false;
    try
    {
        intmap3[0];
    }
    catch(key_error &)
    {
        exception = true;
    }
    assert(exception);

    immutable_map<int, const int> intmap4;
    for(size_t i = 0; i < 10000; ++i)
    {
        intmap4 = intmap4.set(i, i);
		assert(intmap4[i] == i);
    }

    for(size_t i = 0; i < 10000; ++i)
    {
        assert(intmap4[i] == i);
    }

	immutable_map<int, int> intmap5;
	intmap5 = intmap5.set(0, 0);
	assert(intmap5[0] == 0);
	// since we return a non-const reference this is possible
	intmap5[0] = 4;
	assert(intmap5[0] == 4);

    std::cout << "tests complete" << std::endl;
}
