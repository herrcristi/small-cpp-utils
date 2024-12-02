#pragma once

#include <chrono>
#include <thread>

#include <time.h>

namespace small {
    //
    // sleep
    //
    inline void sleep(int time_in_ms)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(time_in_ms));
    }
    inline void sleepMicro(int time_in_microseconds)
    {
        std::this_thread::sleep_for(std::chrono::microseconds(time_in_microseconds));
    }

    //
    // time utils
    //
    inline std::chrono::system_clock::time_point timeNow()
    {
        return std::chrono::system_clock::now();
    }

    inline long long timeDiffMs(std::chrono::system_clock::time_point start, std::chrono::system_clock::time_point end = std::chrono::system_clock::now())
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }
    inline long long timeDiffMicro(std::chrono::system_clock::time_point start, std::chrono::system_clock::time_point end = std::chrono::system_clock::now())
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }
    inline long long timeDiffNano(std::chrono::system_clock::time_point start, std::chrono::system_clock::time_point end = std::chrono::system_clock::now())
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    }

    //
    // to Unix timestamp in milliseconds since 1970
    //
    inline unsigned long long toUnixTimestamp(std::chrono::system_clock::time_point time = std::chrono::system_clock::now())
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()).count();
    }

    //
    // to iso string format '2024-12-02T18:45:43.950Z'
    //
    inline std::string toISOString(std::chrono::system_clock::time_point time = std::chrono::system_clock::now())
    {
        auto tt = std::chrono::system_clock::to_time_t(time);
        std::tm tt_tm; // = *std::gmtime(&tt) is not thread safe
#ifdef _WIN32
        gmtime_s(&&tt_tm, &tt);
#else
        gmtime_r(&tt, &tt_tm);
#endif

        auto timestamp = small::toUnixTimestamp(time);

        std::stringstream ss;
        ss
            << std::setfill('0') << std::put_time(&tt_tm, "%FT%H:%M:")
            << std::setw(2) << (timestamp / 1000) % 60 << '.'
            << std::setw(3) << timestamp % 1000
            << "Z"; // << std::put_time(&tt_tm, "%z");

        return ss.str();
    }

    //
    // time functions for  high_resolution_clock
    //
    inline std::chrono::high_resolution_clock::time_point htimeNow()
    {
        return std::chrono::high_resolution_clock::now();
    }

    inline long long htimeDiffMs(std::chrono::high_resolution_clock::time_point start, std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now())
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }
    inline long long htimeDiffMicro(std::chrono::high_resolution_clock::time_point start, std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now())
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }
    inline long long htimeDiffNano(std::chrono::high_resolution_clock::time_point start, std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now())
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    }

} // namespace small
