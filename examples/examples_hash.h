#pragma once

#include "examples_common.h"

#include "../include/hash.h"

namespace examples::hash {
    //
    // example 1 - Using FNV-1a hash algorithm (secure, collision-resistant)
    //
    inline int Example1()
    {
        std::cout << "Hash: \"some text\" (using FNV-1a algorithm)\n";

        // Using FNV-1a for null-terminated string
        unsigned long long hash = small::qhash1az("some text");

        std::cout << "hash " << hash << "\n";

        // Hash partial string, then continue with null-terminated
        unsigned long long h1 = small::qhash1a("some ", 5 /*strlen*/, 0);
        unsigned long long h2 = small::qhash1az("text", h1);

        std::cout << "hash after multiple rounds " << h2 << "\n";

        std::cout << "Hash finished\n\n";

        return 0;
    }

    //
    // example 2 - Using legacy 131x algorithm (fast but less collision-resistant)
    //
    inline int Example2()
    {
        std::cout << "Hash: \"some text\" (using legacy 131x algorithm)\n";

        unsigned long long hash = small::qhash131z("some text");

        std::cout << "legacy hash " << hash << "\n";

        std::cout << "Legacy Hash finished\n\n";

        return 0;
    }
} // namespace examples::hash