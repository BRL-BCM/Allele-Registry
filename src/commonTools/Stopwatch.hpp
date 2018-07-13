#ifndef STOPWATCH_H_
#define STOPWATCH_H_

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <ostream>
#include <string>


class Stopwatch
{
	private:
		boost::posix_time::ptime lastStart;
		uint64_t summaryTime;  // in microseconds
	public:
		// constructor, start time measurement
		Stopwatch() : lastStart(boost::posix_time::microsec_clock::universal_time()), summaryTime(0)
		{}
		// start time measurement
		void restart()
		{
			this->lastStart = boost::posix_time::microsec_clock::universal_time();
		}
		// save time measurement (update summaryTime), returns measured time in milliseconds, restart time measurement
		uint32_t save_and_restart_ms()
		{
			boost::posix_time::ptime const lNow = boost::posix_time::microsec_clock::universal_time();
			if (lNow <= this->lastStart) {
				this->lastStart = lNow;
				return 0;
			}
			boost::posix_time::time_duration const lDiff = (lNow - this->lastStart);
			this->lastStart = lNow;
			this->summaryTime += lDiff.total_microseconds();
			return ( (lDiff.total_microseconds() + 500) / 1000 );
		}
		// save time measurement (update summaryTime), returns measured time in seconds, restart time measurement
		uint32_t save_and_restart_sec()
		{
			boost::posix_time::ptime const lNow = boost::posix_time::microsec_clock::universal_time();
			if (lNow <= this->lastStart) {
				this->lastStart = lNow;
				return 0;
			}
			boost::posix_time::time_duration const lDiff = (lNow - this->lastStart);
			this->lastStart = lNow;
			this->summaryTime += lDiff.total_microseconds();
			return ( (lDiff.total_milliseconds() + 500) / 1000 );
		}
		// reset object to the initial state, start time measurement
		void reset_and_restart()
		{
			this->lastStart = boost::posix_time::microsec_clock::universal_time();
			this->summaryTime = 0;
		}
		// ----------- return current time
		uint32_t get_time_ms() const // in milliseconds
		{
			boost::posix_time::ptime const lNow = boost::posix_time::microsec_clock::universal_time();
			if (lNow <= this->lastStart) {
				return 0;
			}
			boost::posix_time::time_duration const lDiff = (lNow - this->lastStart);
			return ( (lDiff.total_microseconds() + 500) / 1000 );
		}
		uint32_t get_time_sec() const // in seconds
		{
			boost::posix_time::ptime const lNow = boost::posix_time::microsec_clock::universal_time();
			if (lNow <= this->lastStart) {
				return 0;
			}
			boost::posix_time::time_duration const lDiff = (lNow - this->lastStart);
			return ( (lDiff.total_milliseconds() + 500) / 1000 );
		}
		// -----------  return current summary time (it doesn't include current measurement)
		uint32_t get_summary_time_ms() const  // in milliseconds
		{
			return ( (this->summaryTime + 500) / 1000 );
		}
		uint32_t get_summary_time_sec() const  // in seconds
		{
			return ( (this->summaryTime + 500000) / 1000000 );
		}
};


class StopwatchReporter
{
	private:
		Stopwatch fStopwatch;
		std::string fStopwatchName;
		std::ostream & fOutputStream;
		void start();
	public:
		StopwatchReporter(std::ostream & outputStream, const std::string & name = "");
		StopwatchReporter(const std::string & name = "");
		virtual void reportTime(const std::string & message);
		virtual ~StopwatchReporter();
};


#endif /* STOPWATCH_H_ */
