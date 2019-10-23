#pragma once

#include <mutex>
#include <atomic>
#include <chrono>
#include <condition_variable>

// use it as an event
namespace small
{
    enum class EventType
    {
        kEvent_Automatic,
        kEvent_Manual
    };

    // event class based on mutex
    class event
    {
    public:
        // event
        event                                       ( const EventType& event_type = EventType::kEvent_Automatic ) : event_type_(event_type), event_value_(false){}

        // use it as locker (std::unique_lock<small:event> m...)
        void            lock                        () { lock_.lock();   }
        void            unlock                      () { lock_.unlock(); }
        bool            try_lock                    () { return lock_.try_lock(); }


        // set event 
        void            set_event                   () 
        { 
            { std::unique_lock<std::mutex> mlock( lock_ ); event_value_ = true; } 
        
            if ( event_type_ == EventType::kEvent_Manual ) { condition_.notify_all(); } else { condition_.notify_one(); } 
        }
        
        // reset event
        void            reset_event                 () 
        { 
            { std::unique_lock<std::mutex> mlock( lock_ ); event_value_ = false; }
        }

    
        
        // wait
        void            wait                        () 
        { 
            std::unique_lock<std::mutex> mlock( lock_ ); 
            while ( test_event_and_reset() == false )
                condition_.wait( mlock );
        }

        // wait
        template<typename _Predicate>
        void            wait                        ( _Predicate __p )
        {
            std::unique_lock<std::mutex> mlock( lock_ );
            while ( !__p() )
            {
                if ( event_type_ == EventType::kEvent_Manual && test_event_and_reset() == true )
                {
                    std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
                }
                else
                {
                    while ( test_event_and_reset() == false )
                        condition_.wait( mlock );
                }
            }
        }


        
        // wait for (it uses wait_until)
        template<typename _Rep, typename _Period>
        std::cv_status  wait_for                    ( const std::chrono::duration<_Rep, _Period>& __rtime ) 
        { 
            using __dur = typename std::chrono::system_clock::duration;
            auto __reltime = std::chrono::duration_cast<__dur>(__rtime);
            if ( __reltime < __rtime )
                ++__reltime;
            return wait_until( std::chrono::system_clock::now() + __reltime );
        }

        // wait_for
        template<typename _Rep, typename _Period, typename _Predicate>
        bool            wait_for                    ( const std::chrono::duration<_Rep, _Period>& __rtime, _Predicate __p ) 
        { 
            using __dur = typename std::chrono::system_clock::duration;
            auto __reltime = std::chrono::duration_cast<__dur>(__rtime);
            if ( __reltime < __rtime )
                ++__reltime;
            return wait_until( std::chrono::system_clock::now() + __reltime, std::move( __p ) );
        }




        // wait until
        template<typename _Lock, typename _Clock, typename _Duration>
        std::cv_status  wait_until                  ( const std::chrono::time_point<_Clock, _Duration>& __atime  ) 
        { 
            std::unique_lock<std::mutex> mlock( lock_ ); 
            while ( test_event_and_reset() == false )
            {
                std::cv_status ret = condition_.wait_until( mlock, __atime );
                if ( ret == std::cv_status::timeout )
                    return /*one last check*/test_event_and_reset() == false ? std::cv_status::timeout/*still timeout*/ : std::cv_status::no_timeout;
            }
            return std::cv_status::no_timeout;
        }

        // wait_until
        template<typename _Clock, typename _Duration, typename _Predicate>
        bool            wait_until                  ( const std::chrono::time_point<_Clock, _Duration>& __atime, _Predicate __p ) 
        { 
            std::unique_lock<std::mutex> mlock( lock_ );
            while ( !__p() )
            {
                if ( event_type_ == EventType::kEvent_Manual && test_event_and_reset() == true )
                {
                    // check if timeout
                    if ( std::chrono::system_clock::now() >= __atime )
                        return __p();
                    std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
                }
                else
                {
                    while ( test_event_and_reset() == false )
                    {
                        std::cv_status ret = condition_.wait_until( mlock, __atime );
                        if ( ret == std::cv_status::timeout )
                            return __p();
                    }
                }
            }
            return true;
        }


    private:
        // test event and consume it
        bool            test_event_and_reset        ()
        {
            if ( event_value_/*.load()*/ == true )
            {
                if ( event_type_ == EventType::kEvent_Automatic )
                    event_value_ = false;
                return true;
            }
            return false;
        }



    private:
        // mutex locker
        std::mutex      lock_;
        // condition
        std::condition_variable condition_;
        // for manual event
        EventType       event_type_;
        std::atomic_bool event_value_;
    };
}
