#pragma once

#include <atomic>
#include <deque>
#include <functional>
#include <queue>
#include <unordered_map>
#include <vector>

#pragma warning(disable : 4464) // relative include path contains '..'

#include "../base_lock.h"

namespace small::jobsimpl {
    // a job can be in the following states (order is important because it may progress only to higher states)
    enum class EnumJobsState : unsigned int
    {
        kNone = 0,
        kInProgress,
        kWaitChildren,
        kFinished,
        kTimeout,
        kFailed,
        kCancelled,
    };

    // a job item
    template <typename JobsTypeT, typename JobsRequestT, typename JobsResponseT>
    struct jobs_item
    {
        using JobsID = unsigned long long;

        JobsID                     m_id{};                        // job unique id
        JobsTypeT                  m_type{};                      // job type
        std::atomic<EnumJobsState> m_state{EnumJobsState::kNone}; // job state
        std::atomic<int>           m_progress{};                  // progress 0-100 for state kInProgress
        std::atomic_bool           m_has_parents{};               // for dependencies relationships parent-child
        std::atomic_bool           m_has_children{};              // for dependencies relationships parent-child
        std::vector<JobsID>        m_parentIDs{};                 // for dependencies relationships parent-child
        std::vector<JobsID>        m_childrenIDs{};               // for dependencies relationships parent-child
        JobsRequestT               m_request{};                   // request needed for processing function
        JobsResponseT              m_response{};                  // where the results are saved (for the finished callback if exists)

        explicit jobs_item() = default;

        explicit jobs_item(const JobsID& jobs_id, const JobsTypeT& jobs_type, const JobsRequestT& jobs_request)
            : m_id(jobs_id), m_type(jobs_type), m_request(jobs_request) {}
        explicit jobs_item(const JobsID& jobs_id, const JobsTypeT& jobs_type, JobsRequestT&& jobs_request)
            : m_id(jobs_id), m_type(jobs_type), m_request(std::forward<JobsRequestT>(jobs_request)) {}

        explicit jobs_item(const JobsTypeT& jobs_type, const JobsRequestT& jobs_request)
            : m_type(jobs_type), m_request(jobs_request) {}
        explicit jobs_item(const JobsTypeT& jobs_type, JobsRequestT&& jobs_request)
            : m_type(jobs_type), m_request(std::forward<JobsRequestT>(jobs_request)) {}

        jobs_item(const jobs_item& other) { operator=(other); };
        jobs_item(jobs_item&& other) noexcept { operator=(other); };
        jobs_item& operator=(const jobs_item& other)
        {
            m_id           = other.m_id;
            m_type         = other.m_type;
            m_state        = other.m_state.load();
            m_progress     = other.m_progress.load();
            m_has_parents  = other.m_has_parents.load();
            m_has_children = other.m_has_children.load();
            m_parentIDs    = other.m_parentIDs;
            m_childrenIDs  = other.m_childrenIDs;
            m_request      = other.m_request;
            m_response     = other.m_response;
            return *this;
        }
        jobs_item& operator=(jobs_item&& other) noexcept
        {
            m_id           = std::move(other.m_id);
            m_type         = std::move(other.m_type);
            m_state        = other.m_state.load();
            m_progress     = other.m_progress.load();
            m_has_parents  = other.m_has_parents.load();
            m_has_children = other.m_has_children.load();
            m_parentIDs    = std::move(other.m_parentIDs);
            m_childrenIDs  = std::move(other.m_childrenIDs);
            m_request      = std::move(other.m_request);
            m_response     = std::move(other.m_response);
            return *this;
        }

        //
        // set job state (can only go from lower to upper state)
        //
        inline bool set_state(const EnumJobsState& new_state)
        {
            for (;;) {
                EnumJobsState current_state = m_state.load();
                if (current_state >= new_state) {
                    return false;
                }
                if (m_state.compare_exchange_weak(current_state, new_state)) {
                    return true;
                }
            }
        }

        // clang-format off
        inline bool set_state_none          () { return set_state(EnumJobsState::kNone); }
        inline bool set_state_inprogress    () { return set_state(EnumJobsState::kInProgress); }
        inline bool set_state_waitchildren  () { return set_state(EnumJobsState::kWaitChildren); }
        inline bool set_state_finished      () { return set_state(EnumJobsState::kFinished); }
        inline bool set_state_timeout       () { return set_state(EnumJobsState::kTimeout); }
        inline bool set_state_failed        () { return set_state(EnumJobsState::kFailed); }
        inline bool set_state_cancelled     () { return set_state(EnumJobsState::kCancelled); }
        
        static bool is_state_complete       (const EnumJobsState& state) { return state >= EnumJobsState::kFinished; }

        inline EnumJobsState get_state      () const { return m_state.load(); }
        inline bool is_state                (const EnumJobsState& state) const { return get_state() == state; }
        inline bool is_complete             () const { return is_state_complete(get_state()); }

        inline bool is_state_none           () const { return is_state(EnumJobsState::kNone); }
        inline bool is_state_inprogress     () const { return is_state(EnumJobsState::kInProgress); }
        inline void is_state_waitchildren   () const { return is_state(EnumJobsState::kWaitChildren); }
        inline bool is_state_finished       () const { return is_state(EnumJobsState::kFinished); }
        inline bool is_state_timeout        () const { return is_state(EnumJobsState::kTimeout); }
        inline void is_state_failed         () const { return is_state(EnumJobsState::kFailed); }
        inline void is_state_cancelled      () const { return is_state(EnumJobsState::kCancelled); }
        // clang-format on

        //
        // set job progress (can only increase)
        //
        inline bool set_progress(const int& new_progress)
        {
            for (;;) {
                int current_progress = m_progress.load();
                if (current_progress >= new_progress) {
                    return false;
                }
                if (m_progress.compare_exchange_weak(current_progress, new_progress)) {
                    return true;
                }
            }
        }

        inline int get_progress() const
        {
            return m_progress.load();
        }

        //
        // add child
        //
        inline void add_child(const JobsID& child_jobs_id)
        {
            m_childrenIDs.push_back(child_jobs_id); // this should be set under locked area
            m_has_children = true;
        }

        inline bool has_children() const
        {
            return m_has_children.load();
        }

        //
        // add parent
        //
        inline void add_parent(const JobsID& parent_jobs_id)
        {
            m_parentIDs.push_back(parent_jobs_id); // this should be set under locked area
            m_has_parents = true;
        }

        inline bool has_parents() const
        {
            return m_has_parents.load();
        }
    };

} // namespace small::jobsimpl
