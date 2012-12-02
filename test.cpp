#include <iostream>
#include <cassert>

#include "immutable_map.hpp"

using namespace deepness;

int main(int argc, char *argv[])
{
    immutable_map<int, int> intmap;
    auto it = intmap.find(0);
    assert(it == intmap.end());
}
