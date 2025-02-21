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
    // operations with default processing function
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

        retq = jobs.queue().push_back(JobsType::kJobsSettings, {JobsType::kJobsSettings, 101, "settings101"}, &jobs_id);
        ASSERT_EQ(retq, 1);
        retq = jobs.queue().jobs_start(small::EnumPriorities::kNormal, jobs_id);
        ASSERT_EQ(retq, 1);

        ASSERT_GE(jobs.size(), 2); // because the thread is not started

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
        ASSERT_EQ(processing_count, 2);

        // push after wait is not allowed
        retq = jobs.queue().push_back_and_start(
            small::EnumPriorities::kNormal, JobsType::kJobsSettings, {JobsType::kJobsSettings, 101, "settings101"}, &jobs_id);
        ASSERT_EQ(retq, 0);
        ASSERT_EQ(jobs.size(), 0);
    }

    //
    // operations with default processing function and throtelling (sleep between requests)
    //
    TEST_F(JobsEngineTest, Jobs_Default_Processing_Sleep_Between_Requests)
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

    //
    // operations with default processing function and delayed request
    //
    TEST_F(JobsEngineTest, Jobs_Default_Processing_Delay_Request)
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
        });

        // push
        JobsEng::JobsID jobs_id{};

        auto retq = jobs.queue().push_back_and_start_delay_for(
            std::chrono::milliseconds(300),
            small::EnumPriorities::kNormal, JobsType::kJobsSettings, {JobsType::kJobsSettings, 101, "settings101"}, &jobs_id);
        ASSERT_EQ(retq, 1);

        // push back but dont start
        retq = jobs.queue().push_back(JobsType::kJobsSettings, {JobsType::kJobsSettings, 101, "settings101"}, &jobs_id);
        ASSERT_EQ(retq, 1);
        retq = jobs.queue().jobs_start_delay_for(std::chrono::milliseconds(300), small::EnumPriorities::kNormal, jobs_id);
        ASSERT_EQ(retq, 1);

        ASSERT_GE(jobs.size(), 2); // because the thread is not started

        jobs.start_threads(1); // start thread

        // wait to finish
        auto retw = jobs.wait();
        ASSERT_EQ(retw, small::EnumLock::kExit);

        // check size
        ASSERT_EQ(jobs.size(), 0);

        auto elapsed = small::timeDiffMs(timeStart);
        ASSERT_GE(elapsed, 300 - 1); // due conversion
        ASSERT_EQ(processing_count, 2);
    }

    //
    // operations with default processing function and timeout request
    //
    TEST_F(JobsEngineTest, Jobs_Default_Processing_Timeout_Request)
    {
        auto timeStart = small::timeNow();

        JobsEng::JobsConfig config = m_default_config;
        // set timeout to 300ms
        config.m_types[JobsType::kJobsSettings].m_timeout = std::chrono::milliseconds(300);

        JobsEng jobs(config);

        int  processing_count = 0;
        int  finished_count   = 0;
        bool state_is_timeout = false;

        // setup
        jobs.config_default_function_processing([&processing_count](auto& j /*this jobs engine*/, const auto& jobs_items, auto& jobs_config) {
            for (auto& item : jobs_items) {
                std::ignore = item;
                ++processing_count;
            }
        });

        jobs.config_default_function_finished(
            [&finished_count, &state_is_timeout](auto& j /*this jobs engine*/, const auto& jobs_items) {
                for (auto& item : jobs_items) {
                    state_is_timeout = item->is_state_timeout();
                    ++finished_count;
                }
            });

        // push back but dont start
        JobsEng::JobsID jobs_id{};

        auto retq = jobs.queue().push_back(JobsType::kJobsSettings, {JobsType::kJobsSettings, 101, "settings101"}, &jobs_id);
        ASSERT_EQ(retq, 1);

        ASSERT_GE(jobs.size(), 1); // because the thread is not started

        jobs.start_threads(1); // start thread

        // wait to finish
        auto retw = jobs.wait();
        ASSERT_EQ(retw, small::EnumLock::kExit);

        // check size
        ASSERT_EQ(jobs.size(), 0);

        auto elapsed = small::timeDiffMs(timeStart);
        ASSERT_GE(elapsed, 300 - 1); // due conversion
        ASSERT_EQ(processing_count, 0);
        ASSERT_EQ(state_is_timeout, true);
    }

    //
    // operations with jobs specific functions
    //
    TEST_F(JobsEngineTest, Jobs_Functions)
    {
        JobsEng::JobsConfig config = m_default_config;
        JobsEng             jobs(config);

        int processing_count = 0;
        int finished_count   = 0;

        // setup
        jobs.config_jobs_function_processing(
            JobsType::kJobsSettings,
            [&processing_count](auto& j /*this jobs engine*/, const auto& jobs_items, auto& /* jobs_config */) {
                for (auto& item : jobs_items) {
                    std::ignore = item;
                    ++processing_count;
                }
            });

        jobs.config_jobs_function_finished(
            JobsType::kJobsSettings,
            [&finished_count](auto& j /*this jobs engine*/, const auto& jobs_items) {
                for (auto& item : jobs_items) {
                    std::ignore = item;
                    ++finished_count;
                }
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

        ASSERT_EQ(processing_count, 1);
        ASSERT_EQ(finished_count, 1);
    }

    //
    // operations with priority
    //
    TEST_F(JobsEngineTest, Jobs_Priority)
    {
        JobsEng::JobsConfig config = m_default_config;
        JobsEng             jobs(config);

        int                processing_count = 0;
        std::vector<WebID> processed_web_ids;

        // setup
        jobs.config_jobs_function_processing(
            JobsType::kJobsSettings,
            [&processing_count, &processed_web_ids](auto& j /*this jobs engine*/, const auto& jobs_items, auto& /* jobs_config */) {
                for (auto& item : jobs_items) {
                    auto& [jobs_type, web_id, web_data] = item->m_request;
                    processed_web_ids.push_back(web_id);
                    ++processing_count;
                }
            });

        // push
        JobsEng::JobsID jobs_id{};

        auto retq = jobs.queue().push_back_and_start(
            small::EnumPriorities::kNormal, JobsType::kJobsSettings, {JobsType::kJobsSettings, 101, "settings101"}, &jobs_id);
        ASSERT_EQ(retq, 1);

        retq = jobs.queue().push_back_and_start(
            small::EnumPriorities::kHigh, JobsType::kJobsSettings, {JobsType::kJobsSettings, 102, "settings102"}, &jobs_id);
        ASSERT_EQ(retq, 1);

        ASSERT_GE(jobs.size(), 2); // because the thread is not started

        jobs.start_threads(1); // start thread

        // wait to finish
        auto retw = jobs.wait();
        ASSERT_EQ(retw, small::EnumLock::kExit);

        // check size
        ASSERT_EQ(jobs.size(), 0);

        ASSERT_EQ(processing_count, 2);
        ASSERT_EQ(processed_web_ids.size(), 2);
        ASSERT_EQ(processed_web_ids[0], 102); // high priority first
        ASSERT_EQ(processed_web_ids[1], 101); // normal priority second
    }

    //
    // parent-child relationship start parent and children execute first
    //
    TEST_F(JobsEngineTest, Jobs_Relations_Parent_Start_Children_High)
    {
        JobsEng::JobsConfig config = m_default_config;
        JobsEng             jobs(config);

        int                processing_count = 0;
        std::vector<WebID> processed_web_ids;
        int                finished_count = 0;

        // setup
        jobs.config_jobs_function_processing(
            JobsType::kJobsSettings,
            [&processing_count, &processed_web_ids](auto& j /*this jobs engine*/, const auto& jobs_items, auto& /* jobs_config */) {
                for (auto& item : jobs_items) {
                    auto& [jobs_type, web_id, web_data] = item->m_request;
                    processed_web_ids.push_back(web_id);
                    ++processing_count;
                }
            });

        jobs.config_jobs_function_finished(
            JobsType::kJobsSettings,
            [&finished_count](auto& j /*this jobs engine*/, const auto& jobs_items) {
                for (auto& item : jobs_items) {
                    std::ignore = item;
                    ++finished_count;
                }
            });

        // push
        JobsEng::JobsID jobs_id{};

        auto retq = jobs.queue().push_back_and_start(
            small::EnumPriorities::kNormal, JobsType::kJobsSettings, {JobsType::kJobsSettings, 101, "settings101"}, &jobs_id);
        ASSERT_EQ(retq, 1);

        // high priority for child to execute first
        retq = jobs.queue().push_back_and_start_child(
            jobs_id, small::EnumPriorities::kHigh, JobsType::kJobsSettings, {JobsType::kJobsSettings, 102, "settings102"}, &jobs_id);
        ASSERT_EQ(retq, 1);

        ASSERT_GE(jobs.size(), 2); // because the thread is not started

        jobs.start_threads(1); // start thread

        // wait to finish
        auto retw = jobs.wait();
        ASSERT_EQ(retw, small::EnumLock::kExit);

        // check size
        ASSERT_EQ(jobs.size(), 0);

        ASSERT_EQ(processing_count, 2);
        ASSERT_EQ(processed_web_ids.size(), 2);
        ASSERT_EQ(processed_web_ids[0], 102); // children first
        ASSERT_EQ(processed_web_ids[1], 101); // parent second

        ASSERT_EQ(finished_count, 2);
    }

    TEST_F(JobsEngineTest, Jobs_Relations_Parent_Start_Children_Low)
    {
        JobsEng::JobsConfig config = m_default_config;
        JobsEng             jobs(config);

        int                processing_count = 0;
        std::vector<WebID> processed_web_ids;
        int                finished_count = 0;

        // setup
        jobs.config_jobs_function_processing(
            JobsType::kJobsSettings,
            [&processing_count, &processed_web_ids](auto& j /*this jobs engine*/, const auto& jobs_items, auto& /* jobs_config */) {
                for (auto& item : jobs_items) {
                    auto& [jobs_type, web_id, web_data] = item->m_request;
                    processed_web_ids.push_back(web_id);
                    ++processing_count;
                }
            });

        jobs.config_jobs_function_finished(
            JobsType::kJobsSettings,
            [&finished_count](auto& j /*this jobs engine*/, const auto& jobs_items) {
                for (auto& item : jobs_items) {
                    std::ignore = item;
                    ++finished_count;
                }
            });

        // push
        JobsEng::JobsID jobs_id{};

        auto retq = jobs.queue().push_back_and_start(
            small::EnumPriorities::kNormal, JobsType::kJobsSettings, {JobsType::kJobsSettings, 101, "settings101"}, &jobs_id);
        ASSERT_EQ(retq, 1);

        // low priority for child to execute after the parent
        retq = jobs.queue().push_back_and_start_child(
            jobs_id, small::EnumPriorities::kLow, JobsType::kJobsSettings, {JobsType::kJobsSettings, 102, "settings102"}, &jobs_id);
        ASSERT_EQ(retq, 1);

        ASSERT_GE(jobs.size(), 2); // because the thread is not started

        jobs.start_threads(1); // start thread

        // wait to finish
        auto retw = jobs.wait();
        ASSERT_EQ(retw, small::EnumLock::kExit);

        // check size
        ASSERT_EQ(jobs.size(), 0);

        ASSERT_EQ(processing_count, 2);
        ASSERT_EQ(processed_web_ids.size(), 2);
        ASSERT_EQ(processed_web_ids[0], 101); // parent first
        ASSERT_EQ(processed_web_ids[1], 102); // child second

        ASSERT_EQ(finished_count, 2);
    }

    //
    // parent-child relationship parent not started and children
    //
    TEST_F(JobsEngineTest, Jobs_Relations_Parent_NoStart_Children)
    {
        JobsEng::JobsConfig config = m_default_config;
        JobsEng             jobs(config);

        int                processing_count = 0;
        std::vector<WebID> processed_web_ids;
        int                finished_count = 0;

        // setup
        jobs.config_jobs_function_processing(
            JobsType::kJobsSettings,
            [&processing_count, &processed_web_ids](auto& j /*this jobs engine*/, const auto& jobs_items, auto& /* jobs_config */) {
                for (auto& item : jobs_items) {
                    auto& [jobs_type, web_id, web_data] = item->m_request;
                    processed_web_ids.push_back(web_id);
                    ++processing_count;
                }
            });

        jobs.config_jobs_function_finished(
            JobsType::kJobsSettings,
            [&finished_count](auto& j /*this jobs engine*/, const auto& jobs_items) {
                for (auto& item : jobs_items) {
                    std::ignore = item;
                    ++finished_count;
                }
            });

        // push
        JobsEng::JobsID jobs_id{};

        // parent dont start
        auto retq = jobs.queue().push_back(JobsType::kJobsSettings, {JobsType::kJobsSettings, 101, "settings101"}, &jobs_id);
        ASSERT_EQ(retq, 1);

        // child
        retq = jobs.queue().push_back_and_start_child(
            jobs_id, small::EnumPriorities::kNormal, JobsType::kJobsSettings, {JobsType::kJobsSettings, 102, "settings102"}, &jobs_id);
        ASSERT_EQ(retq, 1);

        ASSERT_GE(jobs.size(), 2); // because the thread is not started

        jobs.start_threads(1); // start thread

        // wait to finish
        auto retw = jobs.wait();
        ASSERT_EQ(retw, small::EnumLock::kExit);

        // check size
        ASSERT_EQ(jobs.size(), 0);

        ASSERT_EQ(processing_count, 2);
        ASSERT_EQ(processed_web_ids.size(), 2);
        ASSERT_EQ(processed_web_ids[0], 102); // children first
        ASSERT_EQ(processed_web_ids[1], 101); // parent second

        ASSERT_EQ(finished_count, 2);
    }

    //
    // parent-child relationship parent with children created in the processing function
    //
    TEST_F(JobsEngineTest, Jobs_Relations_Parent_Create_Children_In_Processing)
    {
        JobsEng::JobsConfig config = m_default_config;
        JobsEng             jobs(config);

        std::atomic<int> processing_count{};
        int              finished_count = 0;

        // setup
        jobs.config_default_function_processing(
            [&processing_count](auto& j /*this jobs engine*/, const auto& jobs_items, auto& /* jobs_config */) {
                for (auto& item : jobs_items) {
                    std::ignore = item;
                    ++processing_count;
                }
            });

        jobs.config_jobs_function_processing(
            JobsType::kJobsApiPost,
            [&processing_count](auto& j /*this jobs engine*/, const auto& jobs_items, auto& /* jobs_config */) {
                for (auto& item : jobs_items) {
                    auto& [jobs_type, web_id, web_data] = item->m_request;

                    j.queue().push_back_and_start_child(
                        item->m_id, small::EnumPriorities::kNormal, JobsType::kJobsDatabase, {JobsType::kJobsDatabase, web_id, web_data});

                    ++processing_count;
                }
            });

        jobs.config_default_function_finished(
            [&finished_count](auto& j /*this jobs engine*/, const auto& jobs_items) {
                for (auto& item : jobs_items) {
                    std::ignore = item;
                    ++finished_count;
                }
            });

        // push
        JobsEng::JobsID jobs_id{};

        // parent start
        auto retq = jobs.queue().push_back_and_start(
            small::EnumPriorities::kNormal, JobsType::kJobsApiPost, {JobsType::kJobsApiPost, 101, "settings101"}, &jobs_id);
        ASSERT_EQ(retq, 1);

        ASSERT_GE(jobs.size(), 1); // because the thread is not started and child is not created yet

        jobs.start_threads(1); // start thread

        // wait to finish
        auto retw = jobs.wait();
        ASSERT_EQ(retw, small::EnumLock::kExit);

        // check size
        ASSERT_EQ(jobs.size(), 0);

        ASSERT_EQ(processing_count, 2);
        ASSERT_EQ(finished_count, 2);
    }

    //
    // force exit
    //
    TEST_F(JobsEngineTest, Jobs_Operations_Force_Exit)
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
        });

        // push back but dont start
        JobsEng::JobsID jobs_id{};

        auto retq = jobs.queue().push_back(JobsType::kJobsSettings, {JobsType::kJobsSettings, 101, "settings101"}, &jobs_id);
        ASSERT_EQ(retq, 1);

        ASSERT_GE(jobs.size(), 1); // because the thread is not started

        jobs.start_threads(1); // start thread

        // wait with timeout
        auto retw = jobs.wait_for(std::chrono::milliseconds(300));
        ASSERT_EQ(retw, small::EnumLock::kTimeout);

        // check size
        ASSERT_EQ(jobs.size(), 1);

        auto elapsed = small::timeDiffMs(timeStart);
        ASSERT_GE(elapsed, 300 - 1); // due conversion
        ASSERT_EQ(processing_count, 0);

        // signal force exit
        timeStart = small::timeNow();

        jobs.signal_exit_force();
        retw = jobs.wait();
        ASSERT_EQ(retw, small::EnumLock::kExit);

        elapsed = small::timeDiffMs(timeStart);
        ASSERT_LE(elapsed, 100);
    }

} // namespace