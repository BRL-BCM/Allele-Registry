#include "Stopwatch.hpp"
#include <iostream>

	// ============================================ StopwatchReporter

	static const char * cDelimeter = "\n===========================================================\n";

	void StopwatchReporter::start()
	{
		fOutputStream << cDelimeter << "Stopwatch report";
		if ( fStopwatchName.size() > 0 ) {
			fOutputStream << " (" << fStopwatchName << ") -> ";
		}
		fOutputStream << "Start" << std::endl;
		fStopwatch.reset_and_restart();
	}

	StopwatchReporter::StopwatchReporter(std::ostream & pOutputStream, const std::string & pName)
		: fStopwatchName(pName), fOutputStream(pOutputStream)
	{
		this->start();
	}

	StopwatchReporter::StopwatchReporter(const std::string & pName)
		: fStopwatchName(pName), fOutputStream(std::cout)
	{
		this->start();
	}

	static void printTime(std::ostream & pOutputStream, uint32_t pTime)
	{
		pOutputStream << pTime / 1000 << ".";
		if ( pTime % 1000 < 100 ) {
			pOutputStream << "0";
			if ( pTime % 1000 < 10 ) {
				pOutputStream << "0";
			}
		}
		pOutputStream << pTime % 1000;
	}

	void StopwatchReporter::reportTime(const std::string & pMessage)
	{
		uint32_t lTime = fStopwatch.save_and_restart_ms();
		fOutputStream << "Stopwatch report";
		if ( fStopwatchName.size() > 0 ) {
			fOutputStream << " (" << fStopwatchName << ") -> ";
		}
		fOutputStream << pMessage << " -> Time = ";
		printTime(fOutputStream, lTime);
		fOutputStream << std::endl;
		fStopwatch.restart();
	}

	StopwatchReporter::~StopwatchReporter()
	{
		fStopwatch.save_and_restart_ms();
		fOutputStream << "Stopwatch report";
		if ( fStopwatchName.size() > 0 ) {
			fOutputStream << " (" << fStopwatchName << ") -> ";
		}
		fOutputStream << "End -> Overal time = ";
		printTime(fOutputStream, fStopwatch.get_summary_time_ms());
		fOutputStream << " seconds" << cDelimeter << std::flush;
	}

