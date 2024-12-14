#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <map>
#include <thread>
#include <unistd.h>

#include "examples/examples_base64.h"
#include "examples/examples_buffer.h"
#include "examples/examples_event.h"
#include "examples/examples_hash.h"
#include "examples/examples_jobs_engine.h"
#include "examples/examples_lock_queue.h"
#include "examples/examples_prio_queue.h"
#include "examples/examples_spinlock.h"
#include "examples/examples_time_queue.h"
#include "examples/examples_util.h"
#include "examples/examples_worker_threads.h"

int main()
{
    std::cout << "Hello. Here are the default small cpp utils examples\n\n";

    examples::spinlock::Example1();
    examples::hash::Example1();
    examples::base64::Example1();
    examples::util::Example1();
    examples::buffer::Example1();
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