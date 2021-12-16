#pragma once
#include <stdint.h>

#if USE_GLFW
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <ml_graphics.h>
#include <ml_head_tracking.h>
#include <ml_perception.h>
#include <ml_lifecycle.h>
#include <ml_logging.h>

#include <exception>
#endif

#if NDTECH_HOLO
#include <wincodec.h>
#include <exception>
#endif 

#if ML_DEVICE
#include <time.h>
#include <cstdlib>
#endif

namespace ndtech
{
    // Helper class for animation and simulation timing.
  class StepTimer
  {
  public:
    ~StepTimer() = default;
    StepTimer& operator=(const StepTimer&) & = default;
    StepTimer(const StepTimer&) = default;
    StepTimer(StepTimer&&) = default;
    StepTimer& operator=(StepTimer&&) & = default;

    StepTimer();

    // Get elapsed time since the previous Update call.
    inline int64_t GetElapsedTicks() const { return m_elapsedTicks; }
    inline double GetElapsedSeconds() const { return TicksToSeconds(m_elapsedTicks); }

    // Get total time since the start of the program.
    inline int64_t GetTotalTicks() const { return m_totalTicks; }
    inline double GetTotalSeconds() const { return TicksToSeconds(m_totalTicks); }

    // Get total number of updates since start of the program.
    inline int GetFrameCount() const { return m_frameCount; }

    // Get the current framerate.
    inline int GetFramesPerSecond() const { return m_framesPerSecond; }

    // Set whether to use fixed or variable timestep mode.
    void SetFixedTimeStep(bool isFixedTimestep);

    // Set how often to call Update when in fixed timestep mode.
    void SetTargetElapsedTicks(long targetElapsed);
    void SetTargetElapsedSeconds(double targetElapsed);


#if USE_GLFW
    // Integer format represents time using 10,000,000 ticks per second.
    static const long TicksPerSecond = 10000000;
#endif

#if NDTECH_HOLO
    // Integer format represents time using 10,000,000 ticks per second.
    static const long TicksPerSecond = 10000000;
#endif 

#if ML_DEVICE
    // Integer format represents time using 1,000,000,000 ticks per second.
    static const long TicksPerSecond = 1000000000;
#endif
    static double TicksToSeconds(long ticks) { return static_cast<double>(ticks) / TicksPerSecond; }
    static int64_t SecondsToTicks(double seconds) { return static_cast<long>(seconds * TicksPerSecond); }

    // Convenient wrapper for QueryPerformanceFrequency. Throws an exception if
    // the call to QueryPerformanceFrequency fails.
    static inline long GetPerformanceFrequency()
    {
#if USE_GLFW
      LARGE_INTEGER freq{};
      if (!QueryPerformanceFrequency(&freq))
      {
        throw std::exception("QueryPerformanceFrequencyFailed");
      }
      return freq.QuadPart;
#endif

#if NDTECH_HOLO
      LARGE_INTEGER freq{};
      if (!QueryPerformanceFrequency(&freq))
      {
        throw std::exception("QueryPerformanceFrequencyFailed");
    }
      long retval = static_cast<long>(freq.QuadPart);
      return retval;
#endif 

#if ML_DEVICE
      struct timespec now;
      clock_gettime(CLOCK_MONOTONIC, &now);
      return now.tv_nsec;
#endif

    }

    // Gets the current number of ticks from QueryPerformanceCounter. Throws an
    // exception if the call to QueryPerformanceCounter fails.
    static inline int64_t GetTicks()
    {
#if USE_GLFW
      LARGE_INTEGER ticks{};
      if (!QueryPerformanceCounter(&ticks))
      {
        throw std::exception("QueryPerformanceFrequencyFailed");
      }
      return int64_t(ticks.QuadPart);
#endif

#if NDTECH_HOLO
      LARGE_INTEGER ticks{};
      if (!QueryPerformanceCounter(&ticks))
      {
        throw std::exception("QueryPerformanceFrequencyFailed");
    }
      return int64_t(ticks.QuadPart);
#endif 

#if ML_DEVICE
      struct timespec now;
      clock_gettime(CLOCK_MONOTONIC, &now);
      return (int64_t)now.tv_sec * 1000000000LL + now.tv_nsec;
#endif
    }

    // After an intentional timing discontinuity (for instance a blocking IO operation)
    // call this to avoid having the fixed timestep logic attempt a set of catch-up
    // Update calls.

    void ResetElapsedTime();

    // Update timer state, calling the specified Update function the appropriate number of times.
    template<typename TUpdate>
    void Tick(const TUpdate& update)
    {
      // Query the current time.
      int64_t currentTime = GetTicks();
      int64_t timeDelta = 0;
      if (!m_paused) {
        timeDelta = currentTime - m_qpcLastTime;

        m_qpcLastTime = currentTime;
        m_qpcSecondCounter += timeDelta;

        // Clamp excessively large time deltas (e.g. after paused in the debugger).
        if (timeDelta > m_qpcMaxDelta)
        {
          timeDelta = m_qpcMaxDelta;
        }

        // Convert QPC units into a canonical tick format. This cannot overflow due to the previous clamp.
        timeDelta *= TicksPerSecond;
        timeDelta /= m_qpcFrequency;

        int lastFrameCount = m_frameCount;

        if (m_isFixedTimeStep)
        {
          // Fixed timestep update logic

          // If the app is running very close to the target elapsed time (within 1/4 of a millisecond) just clamp
          // the clock to exactly match the target value. This prevents tiny and irrelevant errors
          // from accumulating over time. Without this clamping, a game that requested a 60 fps
          // fixed update, running with vsync enabled on a 59.94 NTSC display, would eventually
          // accumulate enough tiny errors that it would drop a frame. It is better to just round
          // small deviations down to zero to leave things running smoothly.

          if (abs(static_cast<long>(timeDelta - m_targetElapsedTicks)) < TicksPerSecond / 4000)
          {
            timeDelta = m_targetElapsedTicks;
          }

          m_leftOverTicks += timeDelta;

          while (m_leftOverTicks >= m_targetElapsedTicks)
          {
            m_elapsedTicks = m_targetElapsedTicks;
            m_totalTicks += m_targetElapsedTicks;
            m_leftOverTicks -= m_targetElapsedTicks;
            m_frameCount++;

            update();
          }
        }
        else
        {
          // Variable timestep update logic.
          m_elapsedTicks = timeDelta;
          m_totalTicks += timeDelta;
          m_leftOverTicks = 0;
          m_frameCount++;

          update();
        }

        // Track the current framerate.
        if (m_frameCount != lastFrameCount)
        {
          m_framesThisSecond++;
        }

        if (m_qpcSecondCounter >= static_cast<long>(m_qpcFrequency))
        {
          m_framesPerSecond = m_framesThisSecond;
          m_framesThisSecond = 0;
          m_qpcSecondCounter %= m_qpcFrequency;
        }
      }
      else {
        m_elapsedTicks = 0;
        update();
      }
    }
    void TogglePause();

  private:

      // Source timing data uses QPC units.
    long m_qpcFrequency;
    int64_t m_qpcLastTime;
    long m_qpcMaxDelta;

    // Derived timing data uses a canonical tick format.
    int64_t m_elapsedTicks;
    int64_t m_totalTicks = 0;
    int64_t m_leftOverTicks;

    // Members for tracking the framerate.
    int m_frameCount;
    int m_framesPerSecond;
    int m_framesThisSecond;
    int64_t m_qpcSecondCounter;

    // Members for configuring fixed timestep mode.
    bool   m_isFixedTimeStep;
    int64_t m_targetElapsedTicks;
    bool m_paused = false;
  };
}
