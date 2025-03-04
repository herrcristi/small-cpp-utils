#if defined(_MSC_VER)
#pragma warning(disable : 4464) // relative include path contains '..'
#pragma warning(disable : 4577) // 'noexcept' used with no exception handling mode specified; termination on exception is not guaranteed
#pragma warning(disable : 4711) // function selected for automatic inline expansion
#pragma warning(disable : 4710) // function not inlined
#pragma warning(disable : 4514) // unreferenced inline function has been removed
#pragma warning(disable : 4577) // 'noexcept' used with no exception handling mode specified; termination on exception is not guaranteed. Specify /EHsc
#pragma warning(disable : 5045) // Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
#endif

#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <map>
#include <thread>

#include "examples/examples_base64.h"
#include "examples/examples_buffer.h"
#include "examples/examples_event.h"
#include "examples/examples_hash.h"
#include "examples/examples_jobs_engine.h"
#include "examples/examples_lock_queue.h"
#include "examples/examples_prio_queue.h"
#include "examples/examples_spinlock.h"
#include "examples/examples_stack_string.h"
#include "examples/examples_time_queue.h"
#include "examples/examples_util.h"
#include "examples/examples_util_timeout.h"
#include "examples/examples_worker_threads.h"

int main()
{
    std::cout << "Hello. Here are the default small cpp utils examples\n\n";

    examples::spinlock::Example1();
    examples::hash::Example1();
    examples::base64::Example1();
    examples::util::Example1();
    examples::utiltimeout::Example1();
    examples::utiltimeout::Example2();
    examples::buffer::Example1();
    examples::stack_string::Example1();
    examples::event::Example1();
    examples::lock_queue::Example1();
    examples::time_queue::Example1();
    examples::prio_queue::Example1();

    examples::worker_thread::Example1();
    examples::worker_thread::Example2();
    examples::worker_thread::Example3_Perf();

    examples::jobs_engine::Example1();

    return 0;
}