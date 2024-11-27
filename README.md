# small

Small project

Contains useful everyday features that can be used in following ways:

-   event (it combines mutex and condition variable to create an event which is either automatic or manual)
-   lock_queue (queue with waiting mechanism to use in concurrent environment)
-   time_queue (creates a queue for delay requests)
-   worker_thread (creates workers on separate threads that do task when requested, based on lock_queue and time_queue)
-   spinlock (or critical_section to do quick locks)

#

-   buffer (a class for manipulating buffers)

#

-   base64 (quick functions for base64 encode & decode)
-   qhash (a quick hash function for buffers and null termination strings)
-   util functions (like small::icasecmp for use with map/set, sleep, timeNow, timeDiff, rand, uuid, ...)

#

For windows if you include windows.h you must undefine small because there is a collision

```
#include <windows.h>
#undef small
```

#

### event

Event is based on mutex and condition_variable

##### !!Important!! An automatic event stay set until it is consumed, a manual event stay set until is reseted

The main functions are

`set_event, reset_event`

`wait, wait_for, wait_until`

Also these functions are available (thanks to mutex)

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

A queue that wait for items until they are available

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

`signal_exit_when_done, is_exit_when_done` // exit when queue is empty

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
// make sure that all calls to wait_* are finished before calling destructor (like in worker_thread)
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

`signal_exit_when_done, is_exit_when_done` // exit when queue is empty

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
// make sure that all calls to wait_* are finished before calling destructor (like in worker_thread)
```

#

### worker_thread

A class that creates several threads for producer/consumer

The following functions are available

For data

`size, empty, clear`

`push_back, emplace_back`

`push_back_delay_for`, `push_back_delay_until`, `emplace_back_delay_for`, `emplace_back_delay_until`

To use it as a locker

`lock, unlock, try_lock`

Signal exit when we no longer want to use worker threads,

`signal_exit_force, is_exit`

`signal_exit_when_done`

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
//

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

`qhash`

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

Use it like this

```
int r = small::stricmp( "a", "C" );
...
std::map<std::string, int, small::icasecmp> m;
```

`sleep`

Use it like this

```
...
small::sleep(100/*ms*/);
...
```

`timeNow, timeDiffMs, timeDiffMicro, timeDiffNano`

Use it like this

```
auto timeStart = small::timeNow();
...
auto elapsed = small::timeDiffMs(timeStart);
...
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
