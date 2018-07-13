#include "../apiDb/TasksManager.hpp"
#include "../commonTools/Stopwatch.hpp"
#include <iostream>
#include <vector>
#include <cstdlib>

TasksManager * tm;

std::vector<unsigned long long> results;

void test_function(unsigned outputIndex)
{
	std::vector<unsigned long long> v(1024*1024);
	unsigned long long a = 0;
	unsigned long long b = 1;
	unsigned const iterCount = 1024*1024;
	for (unsigned iter = 2; iter <= iterCount; ++iter) {
		unsigned long long c = (a + b) % 1024ull*1024*1024*1024*1024*1024;
		a = b;
		b = c;
	}
	results[outputIndex] = b;
}

int test(unsigned cores)
{
	tm = new TasksManager(cores);

	std::vector<unsigned> tasksIds(results.size());

	std::cout << "Test of " << cores << " cores: " << std::flush;
	Stopwatch timer;
	for (unsigned i = 0; i < results.size(); ++i) {
		auto task = [i]() { test_function(i); };
		tasksIds[i] = tm->addTask(task);
	}

	for (unsigned i = 0; i < tasksIds.size(); ++i) {
		tm->joinTask(tasksIds[i]);
	}

	std::cout << timer.get_time_ms() << " ms" << std::endl;

	for (unsigned i = 1; i < results.size(); ++i) {
		if (results[i] != results[i-1]) {
			std::cerr << "The result number " << i << " is incorrect!" << std::endl;
			return -1;
		}
	}

	delete tm;
	return 0;
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		std::cout << "Parameters: max_number_of_cores" << std::endl;
		return 1;
	}
	results.resize(2000);
	Stopwatch timer;
	for (unsigned i = 0; i < results.size(); ++i) {
		test_function(i);
	}
	std::cout << "Control time: " << timer.get_time_ms() << " ms" << std::endl;
	unsigned cores = atoi(argv[1]);
	for (unsigned i = 1; i <= cores; ++i) test(i);
	return 0;
}
