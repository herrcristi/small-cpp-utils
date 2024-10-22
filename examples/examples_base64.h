#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <map>
#include <thread>
#include <unistd.h>

#include "../include/base64.h"

namespace examples::base64 {
    //
    // base64 example 1
    //
    int Example1()
    {
        std::cout << "Base64\n";

        std::string str("hello world");

        std::string b64 = small::tobase64_s(str);
        std::vector<char> vb64 = small::tobase64_v("hello world", 11);

        std::cout << "base64(\"" << str << "\") is " << b64 << "\n";

        std::string decoded = small::frombase64_s(b64);
        std::vector<char> vd64 = small::frombase64_v(b64);

        std::cout << "decoded base64 is \"" << decoded << "\"\n";

        std::cout << "Base64 finished\n\n";

        return 0;
    }
} // namespace examples::base64