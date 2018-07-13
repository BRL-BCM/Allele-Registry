#include "../apiDb/TasksManager.hpp"
#include <queue>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <iostream>

// TODO cut all this stuff and leave only empty functions

	struct TasksManager::Pim {
		std::mutex access;
		unsigned maxNumberOfExecutedTasks;
		unsigned currentNumberOfExecutedTasks;
		unsigned lastTaskId;
		std::queue<std::pair<std::function<void()>,unsigned>> tasks; // function & taskId
		std::unordered_map<std::thread::id,unsigned> tasksBeingProcessed;
		std::unordered_map<unsigned,unsigned> numberOfSubtaskForTask;
		std::condition_variable waitingForTask;
		void processTask(std::function<void()> task, unsigned taskId)
		{
			{
				std::lock_guard<std::mutex> synch_access(access);
				tasksBeingProcessed[std::this_thread::get_id()] = taskId;
			}
			while (true) {
				task();
				{
					std::lock_guard<std::mutex> synch_access(access);
					if (--(numberOfSubtaskForTask[taskId]) == 0) {
						numberOfSubtaskForTask.erase(taskId);
						waitingForTask.notify_all();
					}
					if (tasks.empty()) {
						tasksBeingProcessed.erase(std::this_thread::get_id());
						--currentNumberOfExecutedTasks;
						return; // end of thread
					}
					task = tasks.front().first;
					taskId = tasks.front().second;
					tasks.pop();
					tasksBeingProcessed[std::this_thread::get_id()] = taskId;
				}
			}
		}
	};

	TasksManager::TasksManager(unsigned numberOfCores)
	: pim(new Pim), maxNumberOfConcurrentTasks(numberOfCores)
	{
		pim->maxNumberOfExecutedTasks = numberOfCores;
		pim->currentNumberOfExecutedTasks = 0;
		pim->lastTaskId = 0;
	}

	TasksManager::~TasksManager()
	{
		while (true) {
			{
				std::lock_guard<std::mutex> synch_access(pim->access);
				if (pim->currentNumberOfExecutedTasks == 0) break;
			}
			std::this_thread::yield();
		}
		delete pim;
	}

	unsigned TasksManager::addTask(std::function<void()> task, unsigned priority)
	{
		unsigned taskId;
		{
			std::lock_guard<std::mutex> synch_access(pim->access);
			if (pim->tasksBeingProcessed.count(std::this_thread::get_id())) {
				taskId = pim->tasksBeingProcessed[std::this_thread::get_id()];
			} else {
				taskId = ++(pim->lastTaskId);
			}
			++(pim->numberOfSubtaskForTask[taskId]);
			if (pim->currentNumberOfExecutedTasks >= pim->maxNumberOfExecutedTasks) {
				pim->tasks.push(std::make_pair(task,taskId));
				return taskId;
			}
			++(pim->currentNumberOfExecutedTasks);
		}
		std::thread worker(&Pim::processTask, pim, task, taskId);
		worker.detach();
		return taskId;
	}

	void TasksManager::joinTask(unsigned taskId)
	{
		std::unique_lock<std::mutex> synch_access(pim->access);
		while (pim->numberOfSubtaskForTask.count(taskId)) {
			pim->waitingForTask.wait(synch_access);
		}
	}
