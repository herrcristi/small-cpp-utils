#pragma once

#include "examples_common.h"

#include "../include/stack_string.h"

namespace examples::stack_string {
    using namespace std::literals;

    //
    //  example 1
    //
    inline int Example1()
    {
        std::cout << "Stack String\n";

        small::stack_string s;
        s.clear();

        s = std::string("ancx");
        s = "ancx"sv;
        s = L"ancx"sv;
        s.assign(std::string("ancx"));
        s.set(0, std::string("ancx"));
        s.overwrite(0, std::string("ancx"));

        auto b1 = s < std::string("ancx");
        auto b2 = s + std::string("ancx");
        auto b3 = s == std::string("ancx");

        std::cout << "comparison less with itself = " << b1 << "\n";
        std::cout << "sum b2 = " << b2 << "\n";
        std::cout << "comparison should be equal = " << b3 << "\n";

        std::cout << "assign ancx = " << s << "\n";

        s.set(2 /*from*/, "b", 1);
        std::cout << "assign set b = " << s << "\n";

        s.insert(2 /*from*/, "a", 1);
        std::cout << "assign insert a = " << s << "\n";

        s.insert(10 /*from*/, "c", 1);
        std::cout << "assign insert c = " << s << "\n";

        const char* e = s.data(); // "anabc"
        std::cout << "data = " << e << "\n";

        s.append("hello");
        std::cout << "append = " << s << "\n";
        s.clear();
        std::cout << "after clear = " << s << "\n";

        char* e1 = s.data(); // ""
        std::cout << "data empty = " << e1 << "\n";

        s.append("world", 5);
        std::cout << "append world = " << s << "\n";
        s.set(10, "!", 1);
        std::cout << "append ! = " << s << "\n";

        std::cout << "substr = " << s.substr(2, 2) << "\n";

        auto h = std::hash<std::string_view>{}(s);
        std::cout << "hash = " << h << "\n";

        s.clear();

        small::stack_string<4> s1 = std::string_view{"this is a long string to show switch to std::string"};
        std::cout << "long string = " << s1 << "\n";

        small::stack_string<4> s2  = std::string_view{"s2"};
        s2                        += "make it allocate the std::string";
        std::cout << "long string = " << s2 << "\n";
        std::wcout << "long string utf16 = " << std::wstring_view{s2.c_wstring()} << "\n";

        auto s3 = s + s1;

        std::cout << "Stack String finished\n\n";

        return 0;
    }
} // namespace examples::stack_string