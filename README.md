# immutable_map

Note that this library is currently very early in development.

This is an implementation of an immutable map in c++.
The underlying data-structure is a hash array mapped trie.

The map currently only supports setting, getting and erasing data.

## Usage

The library is header only, so you only need to include the hpp file.

```c++
#include "immutable_map.hpp"


deepness::immutable_map<int, const int> intmap;
auto intmap2 = intmap.set(0, 42);
assert(intmap2[0] == 42);
auto intmap3 = intmap2.erase(0);
try
{
    intmap3[0];
}
catch(deepness::key_error &)
{
    // there is no 0 key in intmap3
}
```

