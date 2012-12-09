# immutable_map

Note that this library is currently very early in development.

This is an implementation of an immutable map in c++
The underlying data-structure is a hash array mapped trie.

The map currently only supports setting and getting data.

## Usage

The library is header only, so you only need to include the hpp file.

```c++
#include "immutable_map.hpp"


deepness::immutable_map<int, int> intmap;
auto intmap2 = intmap.set(0, 42);
assert(intmap2[0] == 42);
```

