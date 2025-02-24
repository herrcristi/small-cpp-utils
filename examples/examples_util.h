#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include <thread>

#include "../include/util.h"

namespace examples::util {
    //
    //  example 1
    //
    inline int Example1()
    {
        std::cout << "Utils\n";

        std::map<std::string, int, small::icasecmp> m;

        constexpr std::string_view b{"B"};
        m[std::string(b)] = 2;

        constexpr const char* a = "a";
        m[a]                    = 1;
        m["A"]                  = 3; // this will have override the "a"

        std::cout << "current map values ";
        for (auto& [key, val] : m) {
            std::cout << "(" << key << ", " << val << ") ";
        }
        std::cout << "\n";

        int r = small::stricmp("a", "A");
        std::cout << "Comparing case insensitive a and A returned " << r << "\n";

        r = small::stricmp("a", "C");
        std::cout << "Comparing case insensitive a and C returned " << r << "\n";

        r = small::stricmp("A", "c");
        std::cout << "Comparing case insensitive A and c returned " << r << "\n";

        // case sensitive
        r = strcmp("a", "C");
        std::cout << "Comparing case sensitive a and C returned " << r << "\n";

        r = strcmp("A", "c");
        std::cout << "Comparing case sensitive A and c returned " << r << "\n";

        std::cout << "Utils finished\n\n";

        return 0;
    }
} // namespace examples::util