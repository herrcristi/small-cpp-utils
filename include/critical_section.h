#pragma once

#include "spinlock.h"

//
// small::critical_section lock;
// ...
// {
//     std::unique_lock<small::spinlock> mlock( lock );
//
//    // do your work
//    ...
// }
//
namespace small {
    using critical_section = spinlock;
}