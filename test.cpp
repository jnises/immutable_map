#include <iostream>
#include <cassert>

#include "immutable_map.hpp"

using namespace deepness;

int main(int argc, char *argv[])
{
    std::cout << "immutable_map tests" << std::endl;
    immutable_map<int, int> intmap;
    auto intmap2 = intmap.set(0, 42);
    assert(intmap2[0] == 42);

    auto intmap3 = intmap2.set(0, 1);
    assert(intmap3[0] == 1);

    immutable_map<int, int> intmap4;
    for(size_t i = 0; i < 10000; ++i)
    {
        intmap4 = intmap4.set(i, i);
		assert(intmap4[i] == i);
    }

    for(size_t i = 0; i < 10000; ++i)
    {
        assert(intmap4[i] == i);
    }

    std::cout << "tests complete" << std::endl;;
}
