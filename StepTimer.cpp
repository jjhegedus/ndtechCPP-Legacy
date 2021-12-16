#include "StepTimer.h"

ndtech::StepTimer::StepTimer() :
  m_elapsedTicks(0),
  m_totalTicks(0),
  m_leftOverTicks(0),
  m_frameCount(0),
  m_framesPerSecond(0),
  m_framesThisSecond(0),
  m_qpcSecondCounter(0),
  m_isFixedTimeStep(false),
  m_targetElapsedTicks(TicksPerSecond / 60)
{ 
  m_qpcFrequency = GetPerformanceFrequency();

  // Initialize max delta to 1/10 of a second.
  m_qpcMaxDelta = m_qpcFrequency / 10;

  m_qpcLastTime = GetTicks();
}


// Set whether to use fixed or variable timestep mode.
void ndtech::StepTimer::SetFixedTimeStep(bool isFixedTimestep) { m_isFixedTimeStep = isFixedTimestep; }

// Set how often to call Update when in fixed timestep mode.
void ndtech::StepTimer::SetTargetElapsedTicks(long targetElapsed) { m_targetElapsedTicks = targetElapsed; }
void ndtech::StepTimer::SetTargetElapsedSeconds(double targetElapsed) { m_targetElapsedTicks = SecondsToTicks(targetElapsed); }

// After an intentional timing discontinuity (for instance a blocking IO operation)
// call this to avoid having the fixed timestep logic attempt a set of catch-up
// Update calls.
void ndtech::StepTimer::ResetElapsedTime()
{
  m_qpcLastTime = GetTicks();

  m_leftOverTicks = 0;
  m_framesPerSecond = 0;
  m_framesThisSecond = 0;
  m_qpcSecondCounter = 0;
}

void ndtech::StepTimer::TogglePause() {
  m_paused = !m_paused;
}