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

    std::cout << "test complete" << std::endl;;
}
