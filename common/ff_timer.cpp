
#include "ff_timer.h"

namespace feifei 
{
#ifdef _WIN32
	void WinTimer::Restart()
	{
		QueryPerformanceFrequency(&cpuFreqHz);
		QueryPerformanceCounter(&startTime);
	}

	void WinTimer::Stop()
	{
		double diffTime100ns;
		QueryPerformanceCounter(&stopTime);
		diffTime100ns = (stopTime.QuadPart - startTime.QuadPart) * 1000.0 / cpuFreqHz.QuadPart;
		ElapsedMilliSec = diffTime100ns / 10.0;
	}
#endif

	void UnixTimer::Restart()
	{
		clock_gettime(CLOCK_MONOTONIC, &startTime);
	}

	void UnixTimer::Stop()
	{
		float diffTime100ns;
		clock_gettime(CLOCK_MONOTONIC, &stopTime);
		double d_startTime = static_cast<double>(startTime.tv_sec)*1e9 + static_cast<double>(startTime.tv_nsec);
		double d_currentTime = static_cast<double>(stopTime.tv_sec)*1e9 + static_cast<double>(stopTime.tv_nsec);
		ElapsedNanoSec = d_currentTime - d_startTime;
		ElapsedMilliSec = ElapsedNanoSec / 1e6;
	}
}
