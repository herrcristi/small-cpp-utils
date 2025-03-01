# small

Small project

Contains useful every day features and also a great material for didactic purposes and learning about usefull data structures

This can be used in following ways:

-   <b>event</b> (it combines mutex and condition variable to create an event which is either automatic or manual)

-   <b>lock_queue</b> (thread safe queue with waiting mechanism to be used in concurrent environment)
-   <b>time_queue</b> (thread safe queue for delay requests)
-   <b>prio_queue</b> (thread safe queue for requests with priority like high, normal, low, etc)

-   <b>worker_thread</b> (creates workers on separate threads that do task when requested, based on lock_queue and time_queue)

-   <b>jobs_engine</b> (uses a thread pool based on worker_thread to process different jobs with config execution pattern)

-   <b>spinlock</b> (or critical_section to do quick locks)

#

-   <b>buffer</b> (a class for manipulating buffers)
-   <b>stack_string</b> (a string that first uses the stack to allocate space, it works faster when using multithreading environment and has conversion to/from wstring)

#

-   <b>base64</b> (quick functions for base64 encode & decode)
-   <b>qhash</b> (a quick hash function for buffers and null termination strings)
-   <b>util</b> functions (like <b>icasecmp</b> for use with map/set, <b>sleep</b>, <b>timeNow</b>, <b>timeDiff</b>, <b>toISOString</b>, <b>rand</b>, <b>uuid</b>, ...)
-   <b>set_timeout</b> and <b>set_interval</b> util functions to execute custom functions after a timeout interval

#

For windows if you include windows.h you must undefine small because there is a collision

```
#include <windows.h>
#undef small
```

#

### event

Event is based on recursive_mutex and condition_variable_any

##### !!Important!! An automatic event is set until it is consumed, a manual event is set until is manually reseted

The main functions are

`set_event, reset_event`

`wait, wait_for, wait_until`

Also these functions are available (thanks to mutex). Can be used multiple times on a thread because the mutex is recursive

`lock, unlock, try_lock`

Use it like this

```
small::event e;
...
{
    std::unique_lock<small::event> mlock( e );
    ...
}
...
e.set_event();
...

// on some thread
e.wait();
// or
e.wait( [&]() -> bool {
    return /*some conditions*/ ? true : false;
} );
...
```

or

```
small::event e( small::EventType::kManual );
...
...
e.set_event();
...

// on some thread
e.wait();
...
// somewhere else
e.reset_event()
```

#

### lock_queue

A queue that waits for items until they are available

The following functions are available

For container

`size, empty, clear, reset`

`push_back, emplace_back`

For events or locking

`lock, unlock, try_lock`

Wait for items

`wait_pop_front, wait_pop_front_for, wait_pop_front_until`

Signal exit when we no longer want to use the queue

`signal_exit_force, is_exit_force` // exit immediatly ignoring what is left in the queue

`signal_exit_when_done, is_exit_when_done` // exit when queue is empty, after this flag is set no more items can be pushed in the queue

Wait for queue to become empty

`wait`, `wait_for`, `wait_until`

Use it like this

```
small::lock_queue<int> q;
...
q.push_back( 1 );
...

// on some thread
int e = 0;
auto ret = q.wait_pop_front( &e );
//auto ret = q.wait_pop_front_for( std::chrono::minutes( 1 ), &e );

// ret can be small::EnumLock::kExit,
// small::EnumLock::kTimeout or small::EnumLock::kElement

if ( ret == small::EnumLock::kElement )
{
     // do something with e
    ...
}

...
// on main thread, no more processing
q.signal_exit_force(); // q.signal_exit_when_done();
...
// make sure that all calls to wait_* are finished before calling destructor (like it is done in worker_thread)
```

#

### time_queue

A queue for delayed items

The following functions are available

For container

`size, empty, clear, reset`

`push_delay_for, emplace_delay_for`

`push_delay_until, emplace_delay_until`

For locking

`lock, unlock, try_lock`

Wait for items

`wait_pop, wait_pop_for, wait_pop_until`

Signal exit when we no longer want to use the queue

`signal_exit_force, is_exit_force` // exit immediatly ignoring what is left in the queue

`signal_exit_when_done, is_exit_when_done` // exit when queue is empty, after this flag is set no more items can be pushed in the queue

Wait for queue to become empty

`wait`, `wait_for`, `wait_until`

Use it like this

```
small::time_queue<int> q;
...
q.push_delay_for( std::chrono::seconds(1), 1 );
...

// on some thread
int e = 0;
auto ret = q.wait_pop( &e );
//auto ret = q.wait_pop_for( std::chrono::minutes( 1 ), &e );

// ret can be small::EnumLock::kExit,
// small::EnumLock::kTimeout or small::EnumLock::kElement

if ( ret == small::EnumLock::kElement )
{
     // do something with e
    ...
}

...
// on main thread, no more processing
q.signal_exit_force(); // q.signal_exit_when_done();
...
// make sure that all calls to wait_* are finished before calling destructor (like it is done in worker_thread)
```

#

### prio_queue

A queue for requests with priority

Works with any user priorities and by default small::EnumPriorities are used (kHighest, kHigh, kNormal, kLow, kLowest)

To avoid antistarvation a config ratio is set, for example 3:1 means that after 3 execution of kHighest there will be 1 execution of kHigh, and so on ...

The following functions are available

For container

`size, empty, clear, reset`

`push_back, emplace_back`

For events or locking

`lock, unlock, try_lock`

Wait for items

`wait_pop_front, wait_pop_front_for, wait_pop_front_until`

Signal exit when we no longer want to use the queue

`signal_exit_force, is_exit_force` // exit immediatly ignoring what is left in the queue

`signal_exit_when_done, is_exit_when_done` // exit when queue is empty, after this flag is set no more items can be pushed in the queue

Wait for queue to become empty

`wait`, `wait_for`, `wait_until`

Use it like this

```
small::prio_queue<int> q;
...
q.push_back( small::EnumPriorities::kNormal, 1 );
...

// on some thread
int e = 0;
auto ret = q.wait_pop_front( &e );
//auto ret = q.wait_pop_front_for( std::chrono::minutes( 1 ), &e );

// ret can be small::EnumLock::kExit,
// small::EnumLock::kTimeout or small::EnumLock::kElement

if ( ret == small::EnumLock::kElement )
{
     // do something with e
    ...
}

...
// on main thread, no more processing
q.signal_exit_force(); // q.signal_exit_when_done();
...
// make sure that all calls to wait_* are finished before calling destructor (like it is done in worker_thread)
```

#

### worker_thread

A class that creates several threads for producer/consumer

Is using the lock_queue and time_queue for convenient functions to push items

The following functions are available

For data

`size, empty, clear`

`push_back, emplace_back`

`push_back_delay_for`, `push_back_delay_until`, `emplace_back_delay_for`, `emplace_back_delay_until`

To use it as a locker

`lock, unlock, try_lock`

Signal exit when we no longer want to use worker threads

`signal_exit_force, is_exit`

`signal_exit_when_done`

Wait for queue to become empty

`wait`, `wait_for`, `wait_until`

Use it like this

```
using qc = std::pair<int, std::string>;
...
// with a lambda for processing working function
small::worker_thread<qc> workers( {.threads_count = 2, .bulk_count = 10}, []( auto& w/*this*/, const auto& vec_items, auto b/*extra param*/ ) -> void
{
    {
        std::unique_lock< small::worker_thread<qc>> mlock( w ); // use worker_thread to lock if needed
        ...
        // for(auto &[i, s]:vec_items) {
        //   std::cout << "thread " << std::this_thread::get_id()
        //             << "processing " << i << " " << s << " b=" << b << "\n";
        // }
    }
}, 5/*extra param*/ );
...
workers.wait(); // manually wait at this point otherwise wait is done in the destructor
...
...
...
using qc = std::pair<int, std::string>;
...
// WorkerThreadFunction can be
struct WorkerThreadFunction
{
    void operator()( small::worker_thread<qc>& w/*worker_thread*/, const std::vector<qc>& items )
    {
        ...
        // add extra in queue
        // w.push_back(...)

        small::sleep(300);
    }
};
...
...
// or like this
small::worker_thread<qc> workers2( {/*default 1 thread*/}, WorkerThreadFunction() );
...
...
workers.push_back( { 1, "a" } );
workers.push_back( std::make_pair( 2, "b" ) );
workers.emplace_back( 3, "e" );
workers.push_back_delay_for( std::chrono::milliseconds(300), { 4, "f" } );
...
// when finishing after signal_exit_force the work is aborted
workers.signal_exit_force(); // workers.signal_exit_when_done();
...
// workers.wait() or will automatically wait on destructor for all threads to finish, also it sets flag exit_when_done and no other pushes are allowed

```

#

### jobs_engine

A class that process different jobs type using the same thread pool

Every job is defined by

-   id
-   type
    -   for each type multiple callback functions can be defined for processing, finishing, child finished
    -   timeout can be setup after which the job is cancelled
-   group
    -   multiple jobs type can be grouped to use same threads, this is configurable (if 1 thread is setup for a group all that job type requests will actually behave like serialized., if 0 threads will mean that some processing will be done outside the jobs engine)
    -   delay between requests (to have throttle) - this can be override in the processing function
-   priority inside a group (high, normal, etc)
-   request / response
-   relationship - one job can be parent for another child job, and by default will be finished when all children are finished (this behaviour can be overriden usign the callbacks)
-   state (in progress, finished, failed, timeout, cancelled, etc)
-   progress

The following functions are available

For data

`size, empty, clear`

`set_config`

`config_default_function_processing, config_default_function_children_finished, config_default_function_finished`

`config_jobs_function_processing, config_jobs_function_children_finished, config_jobs_function_finished`

`queue().push_back_and_start, queue().push_back_and_start_child`

`queue().push_back, queue().push_back_child` <- requires manual start

`queue().push_back_and_start_delay_for, queue().push_back_and_start_delay_until`
`queue().jobs_start_delay_for, queue().jobs_start_delay_until`

`queue().jobs_start, queue().jobs_get`

`push_back_delay_for, push_back_delay_until`

`jobs_parent_child`

To use it as a locker

`lock, unlock, try_lock`

Signal exit when we no longer want to use it,

`signal_exit_force, is_exit`

`signal_exit_when_done`

`wait, wait_for, wait_until`

Use it like this (for a more complete example see the [example](examples/examples_jobs_engine.h) )

```
enum class JobsType
{
    kJobsType1,
    kJobsType2
};
enum class JobsGroupType
{
    kJobsGroup1
};
...
using Request = std::pair<int, std::string>;
using JobsEng = small::jobs_engine<JobsType, Request, int /*response*/, JobsGroupType>;
...
JobsEng::JobsConfig config{
    .m_engine                      = {.m_threads_count = 0 /*dont start any thread yet*/ }, // overall config with default priorities
    .m_groups                      = {
        {JobsGroupType::kJobsGroup1, {.m_threads_count = 1}}}, // config by jobs group
    .m_types = {
        {JobsType::kJobsType1, {.m_group = JobsGroupType::kJobsGroup1}},
        {JobsType::kJobsType2, {.m_group = JobsGroupType::kJobsGroup1}},
    }};
...
// create jobs engine
JobsEng jobs(config);
...
jobs.config_default_function_processing([](auto &j /*this jobs engine*/, const auto &jobs_items) {
    for (auto &item : jobs_items) {
        ...
    }
    ...
});
...
// add specific function for job1 (calling the function from jobs intead of config allows to pass the engine and extra param)
jobs.config_jobs_function_processing(JobsType::kJobsType1, [](auto &j /*this jobs engine*/, const auto &jobs_items, auto b /*extra param b*/) {
    for (auto &item : jobs_items) {
        ...
    }
}, 5 /*param b*/);
...
JobsEng::JobsID              jobs_id{};
std::vector<JobsEng::JobsID> jobs_ids;
...
// push
jobs.queue().push_back(small::EnumPriorities::kNormal, JobsType::kJobsType1, {1, "normal"}, &jobs_id);
...
std::vector<std::shared_ptr<JobsEng::JobsItem>> jobs_items = {
    std::make_shared<JobsEng::JobsItem>(JobsType::kJobsType1, Request{7, "highest"}),
    std::make_shared<JobsEng::JobsItem>(JobsType::kJobsType1, Request{8, "highest"}),
};
jobs.queue().push_back(small::EnumPriorities::kHighest, jobs_items, &jobs_ids);
...
jobs.queue().push_back_delay_for(std::chrono::milliseconds(300), small::EnumPriorities::kNormal, JobsType::kJobsType1, {100, "delay normal"}, &jobs_id);
...
jobs.start_threads(3); // manual start threads
...
// jobs.signal_exit_force();
auto ret = jobs.wait_for(std::chrono::milliseconds(100)); // wait to finished
...
jobs.wait(); // wait here for jobs to finish due to exit flag
...
```

#

### spinlock (like critical_section)

Spinlock is just like a mutex but it uses atomic lockless to do locking (based on std::atomic_flag).

The following functions are available

`lock, unlock, try_lock`

Use it like this

```
small::spinlock lock; // small::critical_section lock;
...
{
    std::unique_lock<small::spinlock> mlock( lock );

    // do your fast work
    ...
}
```

#

## Classes

### buffer

Buffer class for manipulating buffers (not strings)

The following functions are available

`set, append, ...`

and can be used like this

```
small::buffer b;
b.clear();

b.assign( "anc", 3 );                       // "anc"
b.set( 2/*start from*/, "b", 1/*length*/ ); // "anb"

char* e = b.extract();                      // extract "anb"
small::buffer::free( e );                   // free buffer in the class context

small::buffer b1 = { 8192/*chunksize*/, "buffer", 6/*specified length*/ };
small::buffer b2 = { 8192/*chunksize*/, "buffer" };
small::buffer b3 = "buffer";
small::buffer b4 = std::string( "buffer" );

b.append( "hello", 5 );
b.clear( true );

char* e1 = b.extract(); // extract ""
small::buffer::free( e1 );

b.append( "world", 5 );
b.clear();

constexpr std::stringview text{ "hello world" }
std::string s64 = small::tobase64( text );
b.clear();
b = small::frombase64<small::buffer>( s64 );

```

#

### stack_string

A string class that uses the stack to allocate the string (it defines an array basically).
Ofcourse if the string is longer than the stack size a normal std::string is used.

Why? Because in multithreading environment we have a boost in speed by avoiding allocations

The functions from string are also available, and should have the same usage.
Beware that move semantics must copy the part that is allocated on stack.
Also there is conversion from string to wstring and viceversa through ut8.

```
small::stack_string<256/*on stack*/> s;
```

#

## Utilities

### base64

Functions to encode or decode base64

The following functions are available

`tobase64, frombase64`

Use it like this

```
constexpr std::stringview text{ "hello world" }
std::string b64 = small::tobase64( text );
std::vector<char> vb64 = small::tobase64<std::vector<char>>( text );
...
std::string decoded = small::frombase64( b64 );
std::vector<char> vd64 = small::frombase64<std::vector<char>>( b64 );
```

#

### qhash

When you want to do a simple hash

The following function is available

`qhash, qhashz`

Use it like this

```
unsigned long long h = small::qhash( "some text", 9/*strlen(...)*/ );
...
// or you can used like this
unsigned long long h1 = small::qhash( "some ", 5/*strlen(...)*/ );
or
unsigned long long h2 = small::qhashz( "text" /*null terminating string*/,  h1/*continue from h1*/ );
```

#

### util

Utility functions or defines

The following functions are available

`stricmp, struct icasecmp`

`toLowerCase`, `toUpperCase`, `toCapitalizeCase`, `toHex`, `toHexF with 0 prefill`

Use it like this

```
int r = small::stricmp( "a", "C" );
...
std::map<std::string, int, small::icasecmp> m;
...
std::string s = "Some text";
small::toLowerCase(s);

```

`sleep`

Use it like this

```
...
small::sleep(100/*ms*/);
...
```

`timeNow, timeDiffMs, timeDiffMicro, timeDiffNano`

`toUnixTimestamp`, `toISOString`

Use it like this

```
auto timeStart = small::timeNow();
...
auto elapsed = small::timeDiffMs(timeStart);
...
auto timestamp = small::toUnixTimestamp(timeStart);
auto time_str = small::toISOString(timeStart);
```

`rand8, rand16, rand32, rand64`

Use it like this

```
auto r = small::rand64(); // 123123 random number
...
```

`uuidp, uuid, uuidc`

Use it like this

```
auto [r1,r2] = small::uuidp(); // returns a pair of uint64 numbers
...
// returns a uuid as a string "78f202f1bf7a12d46498c9f0e78dd8a3"
auto u = small::uuid();
...
// "{78f202f1-bf7a-12d4-6498-c9f0e78dd8a3}"
auto u1 = small::uuid({.add_hyphen = true, .add_braces = true});
...
// "78F202F1BF7A12D46498C9F0E78DD8A3"
auto uc = small::uuidc(); // return a uuid with capital letters
...
```

### timeout/interval utils

`set_timeout, clear_timeout, set_interval, clear_interval`

Use it like this

```
auto timeoutID = small::set_timeout(std::chrono::milliseconds(1000), [&]() {
    // ....
});
...
// clear only if this should be stopped before execution
/*auto ret =*/ small::clear_timeout(timeoutID);
...
...
auto intervalID = small::set_interval(std::chrono::milliseconds(1000), [&]() {
    // ....
});
...
// clear interval execution
/*auto ret =*/ small::clear_interval(intervalID);
...
...
// cancel everything from the timeout engine
small::timeout::signal_exit_force();
small::timeout::wait();
...
```
