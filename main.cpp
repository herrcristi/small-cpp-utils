#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <map>
#include <thread>
#include <unistd.h>

// make sure the path is included correct
#include "examples/examples_base64.h"
#include "examples/examples_buffer.h"
#include "examples/examples_event.h"
#include "examples/examples_event_queue.h"
#include "examples/examples_hash.h"
#include "examples/examples_spinlock.h"
#include "examples/examples_util.h"

int main()
{
    std::cout << "Hello. Here are the default small cpp utils examples\n\n";

    examples::spinlock::Example1();
    examples::hash::Example1();
    examples::base64::Example1();
    examples::util::Example1();
    examples::buffer::Example1();
    examples::event::Example1();
    examples::event_queue::Example1();

    return 0;
}