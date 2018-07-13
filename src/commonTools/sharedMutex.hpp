#ifndef COMMONTOOLS_SHAREDMUTEX_HPP_
#define COMMONTOOLS_SHAREDMUTEX_HPP_

#include <condition_variable>


class SharedMutex
{
	friend class SharedLockGuard;
	friend class ExclusiveLockGuard;
private:
	std::mutex access;
	unsigned readersCount = 0;
	unsigned writersCount = 0;
	bool writerLock = false;
	std::condition_variable readersCountAndWriterLockWait;
	std::condition_variable writersCountWait;

	void lockShared()
	{
		std::unique_lock<std::mutex> accessGuard(access);
		writersCountWait.wait( accessGuard, [this]()->bool{return (this->writersCount == 0);} );
		++readersCount;
	}

	void unlockShared()
	{
		std::unique_lock<std::mutex> accessGuard(access);
		if (--readersCount == 0) {
			readersCountAndWriterLockWait.notify_one();
		}
	}

	void lockExclusive()
	{
		std::unique_lock<std::mutex> accessGuard(access);
		++writersCount;
		readersCountAndWriterLockWait.wait( accessGuard, [this]()->bool{return (this->readersCount == 0 && this->writerLock == false);} );
		writerLock = true;
	}

	void unlockExclusive()
	{
		std::unique_lock<std::mutex> accessGuard(access);
		writerLock = false;
		if (--writersCount == 0) {
			writersCountWait.notify_all();
		} else {
			readersCountAndWriterLockWait.notify_one();
		}
	}
};


class SharedLockGuard
{
private:
	SharedMutex & access;
public:
	inline SharedLockGuard(SharedMutex & m) : access(m) { access.lockShared(); }
	inline ~SharedLockGuard() { access.unlockShared(); }
};

class ExclusiveLockGuard
{
private:
	SharedMutex & access;
public:
	inline ExclusiveLockGuard(SharedMutex & m) : access(m) { access.lockExclusive(); }
	inline ~ExclusiveLockGuard() { access.unlockExclusive(); }
};



#endif /* COMMONTOOLS_SHAREDMUTEX_HPP_ */
