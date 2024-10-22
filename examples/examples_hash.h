#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <map>
#include <thread>
#include <unistd.h>

#include "../include/hash.h"

namespace examples::hash {
    //
    // hash example 1
    //
    int Example1()
    {
        std::cout << "Hash: \"some text\"\n";

        unsigned long long hash = small::quick_hash_z("some text", 0);

        std::cout << "hash " << hash << "\n";

        unsigned long long h1 = small::quick_hash_b("some ", 5 /*strlen*/, 0);
        unsigned long long h2 = small::quick_hash_z("text", h1);

        std::cout << "hash after multiple rounds " << h2 << "\n";

        std::cout << "hash finishing\n\n";

        return 0;
    }
} // namespace examples::hash