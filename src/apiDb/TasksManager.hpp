#ifndef APIDB_TASKSMANAGER_HPP_
#define APIDB_TASKSMANAGER_HPP_

#include <functional>

class TasksManager
{
private:
	struct Pim;
	Pim * pim;
public:
	unsigned const maxNumberOfConcurrentTasks;
	TasksManager(unsigned numberOfTasks);
	~TasksManager();
	unsigned addTask(std::function<void()> task, unsigned priority = 1); // returns taskId
	void joinTask(unsigned taskId);
	void printState();
};

#endif /* APIDB_TASKSMANAGER_HPP_ */
