#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <map>
#include <thread>
#include <unistd.h>

#include "../include/util.h"

namespace examples::util {
    //
    //  example 1
    //
    int Example1()
    {
        std::cout << "Utils\n";

        std::map<std::string, int, small::icasecmp> m;
        // m["B"] = 2;
        // m["a"] = 1;

        int r = small::stricmp("a", "A");
        std::cout << "Comparing a and A returned " << r << "\n";

        std::cout << "Utils finished\n\n";

        return 0;
    }
} // namespace examples::util