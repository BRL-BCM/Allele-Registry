#ifndef SEM_LIST_H__
#define SEM_LIST_H__

#include "semaphore.h"

// semaphore protected list
template<typename T>
class semList
{
private:
	semaphore emptySem; // initial count 0
	semaphore fullSem; // initial count 10
	std::list<T> list;
	bool _purge = false;

public:
	std::mutex _mutex; 

	semList() {

	}

	void init() {
		// initial count 10
		fullSem.init(10, "full");
		// initial count 0
		emptySem.init(0, "empty");
	}

	void push_back(T elem, semaphore::abortCallback callback) {
		_mutex.lock();
		if(_purge) {
			_mutex.unlock();
			return;
		}
		_mutex.unlock();
		// fullSem --
		// this semaphore will block when list has maximum size of 8
		if(fullSem.acquire(callback) == false) {
			purge();
		}
		// make sure we push to list first, 
		// since there could already be another thread waiting on pop_front semaphore
		_mutex.lock();
		if(_purge == false) {
			list.push_back(elem);
		}
		_mutex.unlock();
		// emptySem ++
		emptySem.release();
	}

	// make sure this function is called in a thread-safe way
	// protect with this classes mutex
	bool empty() {
		bool ret = list.empty();
		return ret;
	}

	// make sure this function is called in a thread-safe way
	// protect with this classes mutex
	T & back() {
		if(_purge) {
			T t;
			return t;
		}
		T & ret = list.back();
		return ret;
	}

	void purge() {
		_mutex.lock();
		_purge = true;
		list.clear();
		_mutex.unlock();
		emptySem.purge();
		fullSem.purge();
	}

	T pop_front(semaphore::abortCallback callback) {
		_mutex.lock();
		if(_purge) {
			_mutex.unlock();
			T t;
			return t;
		}
		_mutex.unlock();
		// emptySem --
		// will block if sem is count zero, 
		// impossible to decrement to negative number, 
		// preventing access to empty list
		if(emptySem.acquire(callback) == false) {
			purge();
                }
		_mutex.lock();
		T elem;
		if(list.empty() == false) {
			elem = std::move(list.front());
			list.pop_front();
		}
		_mutex.unlock();
		// fullSem ++
		// let fullSem know we have more space
		fullSem.release();
		return elem;
	}

	~semList() {
		purge();
	}

};

#endif
