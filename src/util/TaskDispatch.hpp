#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#include "util/NoCopy.hpp"

class TaskDispatch
{
public:
    TaskDispatch( size_t workers, const char* name );
    ~TaskDispatch();

    NoCopy( TaskDispatch );

    void WaitInit();

    void Queue( const std::function<void(void)>& f );
    void Queue( std::function<void(void)>&& f );

    void Sync();

private:
    void Worker();
    void SetName( const char* name, size_t num );

    std::vector<std::function<void(void)>> m_queue;
    std::mutex m_queueLock;
    std::condition_variable m_cvWork, m_cvJobs;
    std::atomic<bool> m_exit;
    size_t m_jobs;

    std::vector<std::thread> m_workers;

    std::thread m_init;
    std::mutex m_initLock;
    std::atomic<bool> m_initDone;
};
