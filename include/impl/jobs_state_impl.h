#pragma once

#include <unordered_map>

#include "jobs_item_impl.h"

namespace small::jobsimpl {

    //
    // helper class for jobs_engine for job item state (parent caller must implement 'jobs_get', 'jobs_completed')
    //
    template <typename JobsTypeT, typename JobsRequestT, typename JobsResponseT, typename ParentCallerT>
    class jobs_state_impl
    {
    public:
        using JobsItem = typename small::jobsimpl::jobs_item<JobsTypeT, JobsRequestT, JobsResponseT>;
        using JobsID   = typename JobsItem::JobsID;

        //
        // jobs_state_impl
        //
        explicit jobs_state_impl(ParentCallerT &parent_caller)
            : m_parent_caller(parent_caller)
        {
        }

        ~jobs_state_impl() = default;

        // clang-format off
        // use it as locker (std::unique_lock<small:jobs_state_impl<T>> m...)
        inline void     lock        () { m_parent_caller.lock(); }
        inline void     unlock      () { m_parent_caller.unlock(); }
        inline bool     try_lock    () { return m_parent_caller.try_lock(); }
        // clang-format on

        //
        // set jobs progress
        //
        inline bool jobs_progress(const JobsID &jobs_id, const int &progress)
        {
            return jobs_progress(jobs_get(jobs_id), progress);
        }

        inline bool jobs_progress(std::shared_ptr<JobsItem> jobs_item, const int &progress)
        {
            if (!jobs_item) {
                return false;
            }
            jobs_item->set_progress(progress);
            if (progress == 100) {
                jobs_state(jobs_item, small::jobsimpl::EnumJobsState::kFinished);
            }
            return true;
        }

        //
        // set jobs response
        //
        inline bool jobs_response(const JobsID &jobs_id, const JobsResponseT &jobs_response)
        {
            std::unique_lock l(*this);
            return jobs_response(jobs_get(jobs_id), jobs_response);
        }

        inline bool jobs_response(const JobsID &jobs_id, JobsResponseT &&jobs_response)
        {
            std::unique_lock l(*this);
            return jobs_response(jobs_get(jobs_id), std::forward<JobsResponseT>(jobs_response));
        }

        inline bool jobs_response(std::shared_ptr<JobsItem> jobs_item, const JobsResponseT &jobs_response)
        {
            if (!jobs_item) {
                return false;
            }
            std::unique_lock l(*this);
            jobs_item->m_response = jobs_response;
            return true;
        }

        inline bool jobs_response(std::shared_ptr<JobsItem> jobs_item, JobsResponseT &&jobs_response)
        {
            if (!jobs_item) {
                return false;
            }
            std::unique_lock l(*this);
            jobs_item->m_response = std::move(jobs_response);
            return true;
        }

        //
        // set jobs finished
        //
        inline bool jobs_finished(const JobsID &jobs_id)
        {
            return jobs_state(jobs_get(jobs_id), small::jobsimpl::EnumJobsState::kFinished);
        }
        inline bool jobs_finished(const JobsID &jobs_id, const JobsResponseT &jobs_response)
        {
            return jobs_state(jobs_get(jobs_id), small::jobsimpl::EnumJobsState::kFinished, jobs_response);
        }
        inline bool jobs_finished(const JobsID &jobs_id, JobsResponseT &&jobs_response)
        {
            return jobs_state(jobs_get(jobs_id), small::jobsimpl::EnumJobsState::kFinished, std::forward<JobsResponseT>(jobs_response));
        }
        inline std::size_t jobs_finished(const std::vector<JobsID> &jobs_ids)
        {
            return jobs_state(jobs_get(jobs_ids), small::jobsimpl::EnumJobsState::kFinished);
        }

        //
        // set jobs failed
        //
        inline bool jobs_failed(const JobsID &jobs_id)
        {
            return jobs_state(jobs_get(jobs_id), small::jobsimpl::EnumJobsState::kFailed);
        }
        inline bool jobs_failed(const JobsID &jobs_id, const JobsResponseT &jobs_response)
        {
            return jobs_state(jobs_get(jobs_id), small::jobsimpl::EnumJobsState::kFailed, jobs_response);
        }
        inline bool jobs_failed(const JobsID &jobs_id, JobsResponseT &&jobs_response)
        {
            return jobs_state(jobs_get(jobs_id), small::jobsimpl::EnumJobsState::kFailed, std::forward<JobsResponseT>(jobs_response));
        }
        inline std::size_t jobs_failed(const std::vector<JobsID> &jobs_ids)
        {
            return jobs_state(jobs_get(jobs_ids), small::jobsimpl::EnumJobsState::kFailed);
        }

        //
        // set jobs cancelled
        //
        inline bool jobs_cancelled(const JobsID &jobs_id)
        {
            return jobs_state(jobs_get(jobs_id), small::jobsimpl::EnumJobsState::kCancelled);
        }
        inline bool jobs_cancelled(const JobsID &jobs_id, const JobsResponseT &jobs_response)
        {
            return jobs_state(jobs_get(jobs_id), small::jobsimpl::EnumJobsState::kCancelled, jobs_response);
        }
        inline bool jobs_cancelled(const JobsID &jobs_id, JobsResponseT &&jobs_response)
        {
            return jobs_state(jobs_get(jobs_id), small::jobsimpl::EnumJobsState::kCancelled, std::forward<JobsResponseT>(jobs_response));
        }
        inline std::size_t jobs_cancelled(const std::vector<JobsID> &jobs_ids)
        {
            return jobs_state(jobs_get(jobs_ids), small::jobsimpl::EnumJobsState::kCancelled);
        }

        //
        // set jobs timeout
        //
        inline bool jobs_timeout(const JobsID &jobs_id)
        {
            return jobs_state(jobs_get(jobs_id), small::jobsimpl::EnumJobsState::kTimeout);
        }
        inline bool jobs_timeout(const JobsID &jobs_id, const JobsResponseT &jobs_response)
        {
            return jobs_state(jobs_get(jobs_id), small::jobsimpl::EnumJobsState::kTimeout, jobs_response);
        }
        inline bool jobs_timeout(const JobsID &jobs_id, JobsResponseT &&jobs_response)
        {
            return jobs_state(jobs_get(jobs_id), small::jobsimpl::EnumJobsState::kTimeout, std::forward<JobsResponseT>(jobs_response));
        }
        inline std::size_t jobs_timeout(const std::vector<JobsID> &jobs_ids)
        {
            // set the jobs as timeout if it is not finished until now (called from queue)
            return jobs_state(jobs_get(jobs_ids), small::jobsimpl::EnumJobsState::kTimeout);
        }

        //
        // set jobs waitforchildren
        //
        inline bool jobs_waitforchildren(const JobsID &jobs_id)
        {
            return jobs_state(jobs_get(jobs_id), small::jobsimpl::EnumJobsState::kWaitChildren);
        }
        inline std::size_t jobs_waitforchildren(const std::vector<std::shared_ptr<JobsItem>> &jobs_items)
        {
            // set the jobs as waitforchildren only if there are children otherwise advance to finish
            return jobs_state(jobs_items, small::jobsimpl::EnumJobsState::kWaitChildren);
        }

        //
        // set jobs state
        //
        inline bool jobs_state(const JobsID &jobs_id, const small::jobsimpl::EnumJobsState &state)
        {
            return jobs_state(jobs_get(jobs_id), state);
        }
        inline bool jobs_state(const JobsID &jobs_id, const small::jobsimpl::EnumJobsState &state, const JobsResponseT &response)
        {
            return jobs_state(jobs_get(jobs_id), state, response);
        }
        inline bool jobs_state(const JobsID &jobs_id, const small::jobsimpl::EnumJobsState &state, JobsResponseT &&response)
        {
            return jobs_state(jobs_get(jobs_id), state, std::forward<JobsResponseT>(response));
        }
        inline std::size_t jobs_state(const std::vector<JobsID> &jobs_ids, const small::jobsimpl::EnumJobsState &state)
        {
            return jobs_state(jobs_get(jobs_ids), state);
        }

        //
        // apply state
        // return true if the state was changed
        //
        inline bool jobs_state(std::shared_ptr<JobsItem> jobs_item, const small::jobsimpl::EnumJobsState &state)
        {
            if (!jobs_item) {
                return false;
            }
            small::jobsimpl::EnumJobsState set_state = state;

            auto ret = jobs_apply_state(jobs_item, state, &set_state);
            if (ret && JobsItem::is_state_complete(set_state)) {
                jobs_completed(jobs_item);
            }
            return ret;
        }

        inline bool jobs_state(std::shared_ptr<JobsItem> jobs_item, const small::jobsimpl::EnumJobsState &state, const JobsResponseT &response)
        {
            jobs_response(jobs_item, response);
            return jobs_state(jobs_item, state);
        }
        inline bool jobs_state(std::shared_ptr<JobsItem> jobs_item, const small::jobsimpl::EnumJobsState &state, JobsResponseT &&response)
        {
            jobs_response(jobs_item, std::forward<JobsResponseT>(response));
            return jobs_state(jobs_item, state);
        }

        inline std::size_t jobs_state(const std::vector<std::shared_ptr<JobsItem>> &jobs_items, const small::jobsimpl::EnumJobsState &jobs_state)
        {
            small::jobsimpl::EnumJobsState         set_state = jobs_state;
            std::vector<std::shared_ptr<JobsItem>> completed_items;
            completed_items.reserve(jobs_items.size());

            std::size_t changed_count{};
            for (auto &jobs_item : jobs_items) {
                auto ret = jobs_apply_state(jobs_item, jobs_state, &set_state);
                if (ret) {
                    ++changed_count;
                    if (JobsItem::is_state_complete(set_state)) {
                        completed_items.push_back(jobs_item);
                    }
                }
            }

            jobs_completed(completed_items);
            return changed_count;
        }

        //
        // get children states
        // compute state and progress based on children
        //      if at least one child has failed/timeout/cancelled then parent is set to failed
        //      else if all children are finished then the parent is finished
        //      else parent is set to wait for children (at least one child is in progress)
        //
        inline void get_children_states(std::shared_ptr<JobsItem> jobs_parent, small::jobsimpl::EnumJobsState *jobs_state, int *jobs_progress)
        {
            // get all children
            auto all_children = jobs_get(jobs_parent->m_childrenIDs);

            // compute state & progress
            std::size_t count_progress           = 0;
            std::size_t count_failed_children    = 0;
            std::size_t count_completed_children = 0;
            std::size_t count_total_children     = all_children.size();

            for (auto &child_jobs_item : all_children) {
                if (!child_jobs_item->is_complete()) {
                    // is in progress
                    count_progress += child_jobs_item->get_progress();
                    continue;
                }

                count_progress += 100;
                ++count_completed_children;
                if (!child_jobs_item->is_state_finished()) {
                    ++count_failed_children;
                }
            }

            // if at least one child has failed/timeout/cancelled then parent is set to failed
            if (count_failed_children) {
                if (jobs_state) {
                    *jobs_state = small::jobsimpl::EnumJobsState::kFailed;
                }
                if (jobs_progress) {
                    *jobs_progress = 100;
                }
                return;
            }

            // if all children are finished then the parent is finished
            if (count_total_children == count_completed_children) {
                if (jobs_state) {
                    *jobs_state = small::jobsimpl::EnumJobsState::kFinished;
                }
                if (jobs_progress) {
                    *jobs_progress = 100;
                }
                return;
            }

            // parent is set to wait for children (at least one child is in progress)
            if (jobs_state) {
                *jobs_state = small::jobsimpl::EnumJobsState::kWaitChildren;
            }
            if (jobs_progress) {
                *jobs_progress = count_progress / (int)count_total_children; // will not be zero due to count_total_children == count_completed_children
            }
        }

    private:
        //
        // apply current state
        //
        inline bool jobs_apply_state(std::shared_ptr<JobsItem> jobs_item, const small::jobsimpl::EnumJobsState &jobs_state, small::jobsimpl::EnumJobsState *jobs_set_state)
        {
            *jobs_set_state = jobs_state;
            // state is already the same
            if (jobs_item->is_state(*jobs_set_state)) {
                return false;
            }

            // set the jobs as timeout only if it is not finished until now
            if (*jobs_set_state == small::jobsimpl::EnumJobsState::kTimeout && jobs_item->is_state_finished()) {
                return false;
            }

            // set the jobs as waitforchildren only if there are children otherwise advance to finish
            if (*jobs_set_state == small::jobsimpl::EnumJobsState::kWaitChildren && !jobs_item->has_children()) {
                *jobs_set_state = small::jobsimpl::EnumJobsState::kFinished;
            }

            jobs_item->set_state(*jobs_set_state);
            return jobs_item->is_state(*jobs_set_state);
        }

        //
        // functions to call the parent
        //
        inline std::shared_ptr<JobsItem> jobs_get(const JobsID &jobs_id)
        {
            return m_parent_caller.jobs_get(jobs_id);
        }

        inline std::vector<std::shared_ptr<JobsItem>> jobs_get(const std::vector<JobsID> &jobs_ids)
        {
            return m_parent_caller.jobs_get(jobs_ids);
        }

        inline void jobs_completed(std::shared_ptr<JobsItem> jobs_item)
        {
            return m_parent_caller.jobs_completed(jobs_item);
        }

        inline void jobs_completed(const std::vector<std::shared_ptr<JobsItem>> &jobs_items)
        {
            return m_parent_caller.jobs_completed(jobs_items);
        }

    private:
        // some prevention
        jobs_state_impl(const jobs_state_impl &)            = delete;
        jobs_state_impl(jobs_state_impl &&)                 = delete;
        jobs_state_impl &operator=(const jobs_state_impl &) = delete;
        jobs_state_impl &operator=(jobs_state_impl &&__t)   = delete;

    private:
        //
        // members
        //
        ParentCallerT &m_parent_caller; // parent jobs engine
    };
} // namespace small::jobsimpl
