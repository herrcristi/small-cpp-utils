
#include <cstdio>
#include <iostream>
#include <map>
#include <thread>

#include "../include/buffer.h"

namespace examples::buffer {
    //
    //  example 1
    //
    int Example1()
    {
        std::cout << "Buffer\n";

        small::buffer b;
        b.clear();

        b.assign(std::string("ancx"));
        std::cout << "assign ancx = " << b << "\n";

        b.set(2 /*from*/, "b", 1);
        std::cout << "assign set b = " << b << "\n";

        b.insert(2 /*from*/, "a", 1);
        std::cout << "assign insert a = " << b << "\n";

        char *e = b.extract(); // extract "anab"
        std::cout << "extract = " << e << "\n";
        small::buffer::free(e);

        b.append("hello");
        std::cout << "append = " << b << "\n";
        b.clear();
        std::cout << "after clear = " << b << "\n";

        char *e1 = b.extract(); // extract ""
        std::cout << "extracting empty = " << e1 << "\n";
        small::buffer::free(e1);

        b.append("world", 5);
        std::cout << "append world = " << b << "\n";

        std::cout << "substr = " << b.substr(2, 2) << "\n";

        auto h = std::hash<std::string_view>{}(b);
        std::cout << "hash = " << h << "\n";

        b.clear();

        std::cout << "Buffer finished\n\n";

        return 0;
    }
} // namespace examples::buffer