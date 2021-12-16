#include "pch.h"
#include "Scheduler.h"

#include <algorithm>

namespace ndtech {

  Scheduler::Scheduler()
    :m_wakeTime(system_clock::now()) {
    m_thread = std::thread{ &Scheduler::Run, this };
  }

  Scheduler::Scheduler(std::thread thread)
    : m_thread(std::move(thread)),
    m_wakeTime(system_clock::now()) {
  }

  void Scheduler::Run() {
    while (!m_done) {

      std::unique_lock<std::mutex> lockGuard(m_waitMutex);

      while (m_wakeTime > system_clock::now()) {
        m_conditionVariable.wait_until(lockGuard, m_wakeTime, [this]() {return m_wakeTime <= system_clock::now(); });
      }

      ProcessReadyTasks();

    }

  }

  void Scheduler::ProcessReadyTasks() {
    auto beginProcessingTime = system_clock::now();

    {
      std::lock_guard<std::mutex> lockGuard(m_repeatingTasksMutex);

      // For each repeating tasks where the next time is less than the current processing time
      // run the task
      std::for_each(
        m_repeatingTasks.begin(),
        std::find_if(
          m_repeatingTasks.begin(),
          m_repeatingTasks.end(),
          // check if the next time for the task is now
          [this, beginProcessingTime](std::tuple<std::function<void(void)>, microseconds, time_point<system_clock>, time_point<system_clock>> task) {
            return std::get<2>(task) > beginProcessingTime;
          }),
        // Update the next execution time and run the task
            [](std::tuple<std::function<void(void)>, microseconds, time_point<system_clock>, time_point<system_clock>>& task) {
            std::get<2>(task) = (std::get<2>(task) + std::get<1>(task));
            std::get<0>(task)();
          });

      // Erase repeating tasks where until is less than the current processing time
      m_repeatingTasks.erase(
        m_repeatingTasks.begin(),
        std::find_if(
          m_repeatingTasks.begin(),
          m_repeatingTasks.end(),
          [this, beginProcessingTime](std::tuple<std::function<void(void)>, microseconds, time_point<system_clock>, time_point<system_clock>> task) {
            return std::get<3>(task) > beginProcessingTime;
          })
      );

    }

    std::lock_guard<std::mutex> lockGuard(m_tasksMutex);

    std::for_each(
      m_tasks.begin(),
      std::find_if(
        m_tasks.begin(),
        m_tasks.end(),
        // check if the scheduled time for the task is now or past
        [this, beginProcessingTime](std::pair<std::function<void(void)>, time_point<system_clock>> task) {
          bool returnValue = (task.second > beginProcessingTime);
          return returnValue;
        }),
      // run the task
          [](std::pair<std::function<void(void)>, time_point<system_clock>> task) {
          task.first();
        });

    m_tasks.erase(
      m_tasks.begin(),
      std::find_if(
        m_tasks.begin(),
        m_tasks.end(),
        // check if the scheduled time for the task is now or past
        [this, beginProcessingTime](std::pair<std::function<void(void)>, time_point<system_clock>> task) {
          return task.second > beginProcessingTime;
        })
    );

    if (m_tasks.size() > 0) {
      m_wakeTime = m_tasks[0].second;
    }
    else {
      m_wakeTime = system_clock::now() + 300ms;
    }

    if (m_repeatingTasks.size() > 0) {
      if (std::get<2>(m_repeatingTasks[0]) < m_wakeTime) {
        m_wakeTime = std::get<2>(m_repeatingTasks[0]);
      }
    }

  }

  void Scheduler::AddTask(std::pair<std::function<void(void)>, time_point<system_clock>> task) {
    { // to scope the lock guard
      std::lock_guard<std::mutex> guard(m_tasksMutex);
      m_tasks.push_back(task);

      if (task.second < m_wakeTime) {
        m_wakeTime = task.second;
      }

      std::sort(
        m_tasks.begin(),
        m_tasks.end(),
        [](std::pair<std::function<void(void)>, time_point<system_clock>> l, std::pair<std::function<void(void)>, time_point<system_clock>> r) {
          return l.second < r.second;
        });
    }

    if (m_wakeTime <= system_clock::now()) {
      // signal the thread to wake
      m_conditionVariable.notify_all();
    }
  }

  void Scheduler::AddTask(std::function<void(void)> taskFunction) {

    std::pair<std::function<void(void)>, time_point<system_clock>> task{ taskFunction, system_clock::now() };

    { // to scope the lock guard
      std::lock_guard<std::mutex> guard(m_tasksMutex);
      m_tasks.push_back(task);

      if (task.second < m_wakeTime) {
        m_wakeTime = task.second;
      }

      std::sort(
        m_tasks.begin(),
        m_tasks.end(),
        [](std::pair<std::function<void(void)>, time_point<system_clock>> l, std::pair<std::function<void(void)>, time_point<system_clock>> r) {
          return l.second < r.second;
        });
    }

    if (m_wakeTime <= system_clock::now()) {
      // signal the thread to wake
      m_conditionVariable.notify_all();
    }
  }


  // until set a year in the future if not explicitly set
  void Scheduler::AddRepeatingTask(std::function<void(void)> task, microseconds interval, time_point<system_clock> until = system_clock::now() + 8760h) {

    std::lock_guard<std::mutex> repeatingTasksGuard(m_repeatingTasksMutex);
    time_point<system_clock> nextExecution = system_clock::now() + interval;
    m_repeatingTasks.push_back(std::make_tuple(task, interval, nextExecution, until));

    if (nextExecution < m_wakeTime) {
      m_wakeTime = nextExecution;
    }

    std::sort(
      m_repeatingTasks.begin(),
      m_repeatingTasks.end(),
      [](std::tuple<std::function<void(void)>, microseconds, time_point<system_clock>, time_point<system_clock>> l, std::tuple<std::function<void(void)>, microseconds, time_point<system_clock>, time_point<system_clock>> r) {
        return std::get<2>(l) < std::get<2>(r);
      });

  }

  void Scheduler::Join() {
    this->m_done = true;
    m_thread.join();
  }

}