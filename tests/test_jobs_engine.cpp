#include <gtest/gtest.h>

#include <latch>

#include "../include/jobs_engine.h"
#include "../include/util.h"

namespace {
    class JobsEngineTest : public testing::Test
    {
    protected:
        JobsEngineTest() = default;

        // same as in examples_jobs_engine.h
        enum class JobsType
        {
            kJobsNone = 0,
            kJobsSettings,
            kJobsApiPost,
            kJobsApiGet,
            kJobsApiDelete,
            kJobsDatabase,
            kJobsCache,
        };

        enum class JobsGroupType
        {
            kJobsGroupDefault,
            kJobsGroupApi,
            kJobsGroupDatabase,
            kJobsGroupCache,
        };

        using WebID       = int;
        using WebData     = std::string;
        using WebRequest  = std::tuple<JobsType, WebID, WebData>;
        using WebResponse = WebData;
        using JobsEng     = small::jobs_engine<JobsType, WebRequest, WebResponse, JobsGroupType>;

        JobsEng::JobsConfig m_default_config;

        void SetUp() override
        {
            m_default_config = {
                .m_engine = {.m_threads_count = 0, // dont start any thread yet
                             .m_config_prio   = {.priorities = {{small::EnumPriorities::kHighest, 2},
                                                                {small::EnumPriorities::kHigh, 2},
                                                                {small::EnumPriorities::kNormal, 2},
                                                                {small::EnumPriorities::kLow, 1}}}}, // overall config with default priorities

                // config by jobs group
                .m_groups = {{JobsGroupType::kJobsGroupDefault, {.m_threads_count = 1}},
                             {JobsGroupType::kJobsGroupApi, {.m_threads_count = 1}},
                             {JobsGroupType::kJobsGroupDatabase, {.m_threads_count = 1}},
                             {JobsGroupType::kJobsGroupCache, {.m_threads_count = 0}}}, // no threads

                // config by jobs type
                .m_types = {{JobsType::kJobsSettings, {.m_group = JobsGroupType::kJobsGroupDefault}},
                            {JobsType::kJobsApiPost, {.m_group = JobsGroupType::kJobsGroupApi}},
                            {JobsType::kJobsApiGet, {.m_group = JobsGroupType::kJobsGroupApi}},
                            {JobsType::kJobsApiDelete, {.m_group = JobsGroupType::kJobsGroupApi}},
                            {JobsType::kJobsDatabase, {.m_group = JobsGroupType::kJobsGroupDatabase}},
                            {JobsType::kJobsCache, {.m_group = JobsGroupType::kJobsGroupCache}}}};
        }
        void TearDown() override
        {
        }
    };

    //
    // lock
    //
    TEST_F(JobsEngineTest, Lock)
    {
        JobsEng::JobsConfig config = m_default_config;
        JobsEng             j(config);

        std::latch sync_thread{1};
        std::latch sync_main{1};

        // create thread
        auto thread = std::jthread([](JobsEng& _j, std::latch& _sync_thread, std::latch& _sync_main) {
            std::unique_lock lock(_j);
            _sync_thread.count_down(); // signal that thread is started (and also locked is acquired)
            _sync_main.wait();         // wait that the main finished executing test to proceed further
            _j.lock();                 // locked again on same thread
            small::sleep(300);         // sleep inside lock
            _j.unlock();
        },
                                   std::ref(j), std::ref(sync_thread), std::ref(sync_main));

        // wait for the thread to start
        sync_thread.wait();

        // try to lock and it wont succeed
        auto locked = j.try_lock();
        ASSERT_FALSE(locked);

        // signal thread to proceed further
        auto timeStart = small::timeNow();
        sync_main.count_down();

        // wait for the thread to stop
        while (!locked) {
            small::sleep(1);
            locked = j.try_lock();
        }
        ASSERT_TRUE(locked);

        // unlock
        j.unlock();

        auto elapsed = small::timeDiffMs(timeStart);
        ASSERT_GE(elapsed, 300 - 1);
    }

    //
    // operations
    //
    TEST_F(JobsEngineTest, Jobs_Operations_Default_Processing)
    {
        auto timeStart = small::timeNow();

        JobsEng::JobsConfig config = m_default_config;
        JobsEng             jobs(config);

        int processing_count = 0;

        // setup
        jobs.config_default_function_processing([&processing_count](auto& j /*this jobs engine*/, const auto& jobs_items, auto& /* jobs_config */) {
            for (auto& item : jobs_items) {
                std::ignore = item;
                ++processing_count;
                small::sleep(300);
            }
        });

        // push
        JobsEng::JobsID jobs_id{};

        auto retq = jobs.queue().push_back_and_start(
            small::EnumPriorities::kNormal, JobsType::kJobsSettings, {JobsType::kJobsSettings, 101, "settings101"}, &jobs_id);
        ASSERT_EQ(retq, 1);
        ASSERT_GE(jobs.size(), 1); // because the thread is not started

        jobs.start_threads(1); // start thread

        // wait_until
        auto retw = jobs.wait_for(std::chrono::milliseconds(0));
        ASSERT_EQ(retw, small::EnumLock::kTimeout);

        // wait to finish
        retw = jobs.wait();
        ASSERT_EQ(retw, small::EnumLock::kExit);

        // check size
        ASSERT_EQ(jobs.size(), 0);

        auto elapsed = small::timeDiffMs(timeStart);
        ASSERT_GE(elapsed, 300 - 1); // due conversion
        ASSERT_EQ(processing_count, 1);

        // push after wait is not allowed
        retq = jobs.queue().push_back_and_start(
            small::EnumPriorities::kNormal, JobsType::kJobsSettings, {JobsType::kJobsSettings, 101, "settings101"}, &jobs_id);
        ASSERT_EQ(retq, 0);
        ASSERT_EQ(jobs.size(), 0);
    }

    TEST_F(JobsEngineTest, Jobs_Operations_Default_Processing_Sleep_Between_Requests)
    {
        auto timeStart = small::timeNow();

        JobsEng::JobsConfig config = m_default_config;
        JobsEng             jobs(config);

        int processing_count = 0;

        // setup
        jobs.config_default_function_processing([&processing_count](auto& j /*this jobs engine*/, const auto& jobs_items, auto& jobs_config) {
            for (auto& item : jobs_items) {
                std::ignore = item;
                ++processing_count;
            }
            jobs_config.m_delay_next_request = std::chrono::milliseconds(400);
        });

        // push
        JobsEng::JobsID jobs_id{};

        auto retq = jobs.queue().push_back_and_start(
            small::EnumPriorities::kNormal, JobsType::kJobsSettings, {JobsType::kJobsSettings, 101, "settings101"}, &jobs_id);
        ASSERT_EQ(retq, 1);
        ASSERT_GE(jobs.size(), 1); // because the thread is not started

        jobs.start_threads(1); // start thread

        // wait to finish
        auto retw = jobs.wait();
        ASSERT_EQ(retw, small::EnumLock::kExit);

        // check size
        ASSERT_EQ(jobs.size(), 0);

        auto elapsed = small::timeDiffMs(timeStart);
        ASSERT_GE(elapsed, 400 - 1); // due conversion
        ASSERT_EQ(processing_count, 1);
    }

    // TEST_F(JobsEngineTest, Jobs_Operations_Delayed)
    // {
    //     auto timeStart = small::timeNow();

    //     int processing_count = 0;

    //     // create workers
    //     small::worker_thread<int> workers({.threads_count = 0 /*no threads*/, .bulk_count = 2}, [&processing_count](auto& w /*this*/, const auto& items) {
    //         processing_count++;
    //     });

    //     // push
    //     workers.push_back(4);
    //     workers.push_back_delay_for(std::chrono::milliseconds(300), 5);
    //     ASSERT_GE(workers.size(), 0);
    //     ASSERT_GE(workers.size_delayed(), 1);

    //     workers.start_threads(1); // start thread

    //     // wait to finish
    //     auto ret = workers.wait();
    //     ASSERT_EQ(ret, small::EnumLock::kExit);

    //     // check size
    //     ASSERT_EQ(workers.size(), 0);
    //     ASSERT_EQ(processing_count, 2);

    //     auto elapsed = small::timeDiffMs(timeStart);
    //     ASSERT_GE(elapsed, 300 - 1); // due conversion
    // }

    // TEST_F(JobsEngineTest, Jobs_Operations_Force_Exit)
    // {
    //     auto timeStart = small::timeNow();

    //     struct JobsEngineFunction
    //     {
    //         void operator()(small::worker_thread<int>& w /*worker_thread*/, [[maybe_unused]] const std::vector<int>& items, [[maybe_unused]] int b /*extra param*/)
    //         {
    //             small::sleep(300);
    //             if (w.is_exit()) {
    //                 return;
    //             }
    //             small::sleep(300);
    //             // process item using the workers lock (not recommended)
    //         }
    //     };

    //     // create workers
    //     small::worker_thread<int> workers({/*default 1 thread*/}, JobsEngineFunction(), 5 /*param b*/);

    //     // push
    //     workers.push_back(5);
    //     workers.push_back(6);
    //     small::sleep(100); // wait for the thread to start and execute first sleep

    //     workers.signal_exit_force();
    //     ASSERT_EQ(workers.size(), 1);

    //     // push after exit will not work
    //     auto r_push = workers.push_back(5);
    //     ASSERT_EQ(r_push, 0);
    //     ASSERT_EQ(workers.size(), 1);

    //     // wait to finish
    //     auto ret = workers.wait();
    //     ASSERT_EQ(ret, small::EnumLock::kExit);

    //     // check size
    //     ASSERT_EQ(workers.size(), 1);

    //     // elapsed only 300 and not 600
    //     auto elapsed = small::timeDiffMs(timeStart);
    //     ASSERT_GE(elapsed, 300 - 1); // due conversion
    //     ASSERT_LT(elapsed, 600 - 1); // due conversion
    // }

} // namespace