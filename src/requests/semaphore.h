#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <mutex>
#include <condition_variable>
        
class semaphore {
public:
    typedef std::function<bool()> abortCallback;

    semaphore (unsigned count_ = 0)
    : count(count_)
    {
    }

    void init(unsigned count_, std::string _name) {
        count = count_;
        name = _name;
    }
    
    inline void release() {
        std::unique_lock<std::mutex> lock(mtx);
        count++;
        //notify the waiting thread
        cv.notify_one();
    }
    inline bool acquire(abortCallback callback) {
        std::unique_lock<std::mutex> lock(mtx);
        int iter = 0;
        auto begin = std::chrono::high_resolution_clock::now();
        while(count == 0) {
            //wait on the mutex until notify is called
            if(_purge == false && callback() == false) {
                cv.wait_for(lock, std::chrono::milliseconds(100));
                auto end = std::chrono::high_resolution_clock::now();
                unsigned time = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                if(time > maxTimeout) {
                    purge();
                }
            } else {
                count = 0;
		return false;
            }
            iter++;
        }
        ASSERT(count > 0);
        count--;
        return true;
    }
    void purge() {
        _purge = true;
    }
private:
    std::mutex mtx;
    std::condition_variable cv;
    unsigned count;
    //unsigned maxTimeout = 60000; // about 1 min, if we didn't get any chunks after one minute, purge
    unsigned maxTimeout = -1; // never time out
    bool _purge = false;
    std::string name;
};

#endif // SEMAPHORE_H
