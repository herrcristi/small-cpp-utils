#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <map>
#include <thread>
#include <unistd.h>

// make sure the path is included correct
#include "examples/examples_spinlock.h"

int main()
{
    std::cout << "Hello. Here are the default small cpp utils examples\n\n";

    examples::spinlock::Example1();

    return 0;
}