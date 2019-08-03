#pragma once

#include "ff_basic.h"
#include <time.h>

namespace feifei
{
	class TimerBase
	{
	protected:
		timespec startTime;
		timespec stopTime;

	public:
		virtual void Restart() = 0;
		virtual void Stop() = 0;

		double ElapsedMilliSec = 0;
		double ElapsedNanoSec = 0;
	};

#ifdef _WIN32
	class WinTimer
	{
	public:
		void Restart();
		void Stop();
	};
#endif

	class UnixTimer:public TimerBase
	{
	public:
		void Restart();
		void Stop();
	};
}