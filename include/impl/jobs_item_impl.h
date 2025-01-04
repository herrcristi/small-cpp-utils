#pragma once

#include <atomic>
#include <deque>
#include <functional>
#include <queue>
#include <unordered_map>
#include <vector>

#include "../base_lock.h"

namespace small::jobsimpl {
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

        JobsID    id{};   // job unique id
        JobsTypeT type{}; // job type
        // std::atomic<EnumJobsState> state{};    // job state
        // std::atomic<int>           progress{}; // progress 0-100 for state kInProgress
        EnumJobsState state{};    // job state
        int           progress{}; // progress 0-100 for state kInProgress
        JobsRequestT  request{};  // request needed for processing function
        JobsResponseT response{}; // where the results are saved (for the finished callback if exists)

        // explicit jobs_item() = default;
        // explicit jobs_item(std::initializer_list<jobs_item>) {};
        // jobs_item(const jobs_item &other) { operator=(other); };
        // jobs_item(jobs_item &&other) { operator=(other); };
        // jobs_item &operator=(const jobs_item &other)
        // {
        //     id       = other.id;
        //     type     = other.type;
        //     state    = other.state.load();
        //     progress = other.progress.load();
        //     request  = other.request;
        //     response = other.response;
        //     return *this;
        // }
        // jobs_item &operator=(jobs_item &&other)
        // {
        //     id       = std::move(other.id);
        //     type     = std::move(other.type);
        //     state    = other.state.load();
        //     progress = other.progress.load();
        //     request  = std::move(other.request);
        //     response = std::move(other.response);
        //     return *this;
        // }

        //
        // set job state (can only go from lower to upper state)
        //
        inline void set_state(const EnumJobsState &new_state)
        {
            // for (;;) {
            //     EnumJobsState current_state = state.load();
            //     if (current_state >= new_state) {
            //         return;
            //     }
            //     if (state.compare_exchange_weak(current_state, new_state)) {
            //         return;
            //     }
            // }
        }

        //
        // set job progress (can only increase)
        //
        inline void set_progress(const int &new_progress)
        {
            // for (;;) {
            //     int current_progress = progress.load();
            //     if (current_progress >= new_progress) {
            //         return;
            //     }
            //     if (progress.compare_exchange_weak(current_progress, new_progress)) {
            //         return;
            //     }
            // }
        }
    };

} // namespace small::jobsimpl
