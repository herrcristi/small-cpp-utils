#pragma once

#include <atomic>
#include <deque>
#include <functional>
#include <queue>
#include <unordered_map>
#include <vector>

#include "base_lock.h"

namespace small {
    // a job can be in the following states
    enum class EnumJobsState : unsigned int
    {
        kNone = 0,
        kInProgress,
        kFinished,
        kFailed,
        kCancelled,
        kTimeout
    };

    // a job item
    template <typename JobsTypeT, typename JobsRequestT, typename JobsResponseT>
    struct jobs_item
    {
        using JobsID = unsigned long long;

        JobsID                     id{};       // job unique id
        JobsTypeT                  type{};     // job type
        std::atomic<EnumJobsState> state{};    // job state
        std::atomic<int>           progress{}; // progress 0-100 for state kInProgress
        JobsRequestT               request{};  // request needed for processing function
        JobsResponseT              response{}; // where the results are saved (for the finished callback if exists)

        //
        // set job state (can only go from lower to upper state)
        //
        inline void set_state(const EnumJobsState &new_state)
        {
            for (;;) {
                EnumJobsState current_state = state.load();
                if (current_state >= new_state) {
                    return;
                }
                if (state.compare_exchange_weak(current_state, new_state)) {
                    return;
                }
            }
        }

        //
        // set job progress (can only increase)
        //
        inline void set_progress(const int &new_progress)
        {
            for (;;) {
                int current_progress = progress.load();
                if (current_progress >= new_progress) {
                    return;
                }
                if (progress.compare_exchange_weak(current_progress, new_progress)) {
                    return;
                }
            }
        }
    };

} // namespace small
