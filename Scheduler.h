#pragma once

#include <vector>
#include <functional>
#include <chrono>
#include <condition_variable>
#include <thread>
#include <mutex>


using namespace std::chrono;
using namespace std::chrono_literals;

namespace ndtech {

  struct Scheduler {

    Scheduler();

    Scheduler(std::thread thread);

    void Run();

    void ProcessReadyTasks();

    void AddTask(std::pair<std::function<void(void)>, time_point<system_clock>> task);

    void AddTask(std::function<void(void)> taskFunction);

    void AddRepeatingTask(std::function<void(void)> task, microseconds interval, time_point<system_clock> until);

    void Join();

  private:
    size_t                                                                                                                m_cache_line_size;
    std::thread                                                                                                           m_thread;
    std::mutex                                                                                                            m_tasksMutex;
    std::vector<std::pair<std::function<void(void)>, time_point<system_clock>>>                                           m_tasks;
    std::mutex                                                                                                            m_repeatingTasksMutex;
    std::vector<std::tuple<std::function<void(void)>, microseconds, time_point<system_clock>, time_point<system_clock>>>   m_repeatingTasks;
    time_point<system_clock>                                                                                              m_wakeTime = system_clock::now() + 100ms;
    std::mutex                                                                                                            m_waitMutex;
    std::condition_variable                                                                                               m_conditionVariable;
    bool                                                                                                                  m_done = false;
  };

}