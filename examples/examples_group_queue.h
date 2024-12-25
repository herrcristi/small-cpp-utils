#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <map>
#include <thread>

#include "../include/group_queue.h"

namespace examples::group_queue {
    //
    //  example 1
    //
    int Example1()
    {
        std::cout << "Group Queue example 1\n";

        enum class Type
        {
            kType1
        };
        enum class GroupType
        {
            kGroup1
        };

        small::group_queue<Type, int, GroupType> q;
        q.add_type_group(Type::kType1, GroupType::kGroup1);

        q.push_back(small::EnumPriorities::kNormal, Type::kType1, 1);

        // on some thread
        std::pair<Type, int> e{};
        auto                 ret = q.wait_pop_front(GroupType::kGroup1, &e);
        // or wait_pop_front_for( std::chrono::minutes( 1 ), GroupType:kGroup1, &e );
        // ret can be small::EnumLock::kExit, small::EnumLock::kTimeout or ret == small::EnumLock::kElement
        if (ret == small::EnumLock::kElement) {
            // do something with e
            std::cout << "elem from group q has type " << (int)e.first << ", " << e.second << "\n";
        }

        // on main thread no more processing (aborting work)
        q.signal_exit_force(); // q.signal_exit_when_done()

        std::cout << "Groups Queue example 1 finish\n\n";

        return 0;
    }

} // namespace examples::group_queue