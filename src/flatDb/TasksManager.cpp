#include "../apiDb/TasksManager.hpp"
#include <list>
#include <unordered_map>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "../debug.hpp"

	struct TasksManager::Pim {

		struct Task {
			std::function<void()> call;
			unsigned priority;
			std::list<unsigned>::const_iterator iter;
		};

		std::mutex access;
		unsigned maxNumberOfExecutedTasks;
		unsigned currentNumberOfExecutedTasks;
		unsigned lastTaskId;
		std::map<unsigned, std::list<unsigned>, std::greater<unsigned>> tasksQueue; // priority -> queue of taskId
		std::unordered_map<unsigned,Task> tasksById;

		void processTask(std::function<void()> task)
		{
			while (true) {
				task();
				{
					std::lock_guard<std::mutex> synch_access(access);
					// ===== exit if there is not tasks to process
					if (tasksQueue.empty()) {
						--currentNumberOfExecutedTasks;
						return; // end of thread
					}
					// ===== get tasks ID to process and remove it from the queue
					unsigned const taskId = tasksQueue.begin()->second.front();
					tasksQueue.begin()->second.pop_front();
					if (tasksQueue.begin()->second.empty()) {
						tasksQueue.erase(tasksQueue.begin());
					}
					// ===== get callback and remove task
					task = tasksById[taskId].call;
					tasksById.erase(taskId);
				}
			}
		}
	};


	TasksManager::TasksManager(unsigned pMaxNumberOfConcurrentTasks)
	: pim(new Pim), maxNumberOfConcurrentTasks(pMaxNumberOfConcurrentTasks)
	{
		pim->maxNumberOfExecutedTasks = pMaxNumberOfConcurrentTasks;
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
			taskId = ++(pim->lastTaskId);
			if (pim->currentNumberOfExecutedTasks >= pim->maxNumberOfExecutedTasks) {
				Pim::Task & obj = pim->tasksById[taskId];
				obj.call = task;
				obj.priority = priority;
				pim->tasksQueue[priority].push_back(taskId);
				obj.iter = pim->tasksQueue[priority].end();
				--(obj.iter);
				return taskId;
			}
			++(pim->currentNumberOfExecutedTasks);
		}
		std::thread worker(&Pim::processTask, pim, task);
		worker.detach();
		return taskId;
	}


//	void TasksManager::updatePriority(unsigned taskId, unsigned priority)
//	{
//		std::lock_guard<std::mutex> synch_access(pim->access);
//		auto it = pim->tasksById.find(taskId);
//		if ( it == pim->tasksById.end() || it->second.priority == priority ) return;
//		std::list<unsigned> & newQueue = pim->tasksQueue[priority];
//		std::list<unsigned> & oldQueue = pim->tasksQueue[it->second.priority];
//		it->second.priority = priority;
//		newQueue.splice( newQueue.end(), oldQueue, it->second.iter );
//	}


	void TasksManager::joinTask(unsigned taskId)
	{
		// TODO - is it needed ?
//		std::unique_lock<std::mutex> synch_access(pim->access);
//		while (pim->numberOfSubtaskForTask.count(taskId)) {
//			pim->waitingForTask.wait(synch_access);
//		}
	}

	void TasksManager::printState()
	{
		std::lock_guard<std::mutex> synch_access(pim->access);
		DebugVar(pim->currentNumberOfExecutedTasks);
		DebugVar(pim->tasksQueue.size());
		DebugVar(pim->tasksById.size());
	}
