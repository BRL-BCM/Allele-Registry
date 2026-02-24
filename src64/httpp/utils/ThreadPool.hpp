/*
 * File: ThreadPool.hpp
 * Part of commonpp.
 *
 * Distributed under the 2-clause BSD licence (See LICENCE.TXT file at the
 * project root).
 *
 * Copyright (c) 2015 Thomas Sanchez.  All rights reserved.
 *
 */
#ifndef _HTTPP_UTILS_THREAD_POOL_HPP_
# define _HTTPP_UTILS_THREAD_POOL_HPP_

#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <iostream>

#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>

#include "Thread.hpp"

namespace HTTPP
{
namespace UTILS
{

class ThreadPool
{
public:
    using ThreadInit = std::function<void()>;

public:
    ThreadPool(size_t nb_thread, std::string name = "", size_t nb_services = 1);
    ~ThreadPool();

    enum
    {
        ROUND_ROBIN = -1,
    };

    template <typename Callable>
    void post(Callable&& callable, int service_id = ROUND_ROBIN)
    {
        getService(service_id).post(std::forward<Callable>(callable));
    }

    template <typename Callable>
    void dispatch(Callable&& callable, int service_id = ROUND_ROBIN)
    {
        getService(service_id).dispatch(std::forward<Callable>(callable));
    }

    bool runningInPool() const noexcept;

    void start(ThreadInit fct = ThreadInit());
    void stop();

    boost::asio::io_service& getService(int service_id = ROUND_ROBIN);

    size_t threads() const noexcept;

    template <typename Callable>
    void postAll(Callable callable);

    template <typename Callable>
    void dispatchAll(Callable callable);

    int get_avail() {
      m.lock();
      int ret = current_service_;
      busy_[ret] = true;
      bool found_avail = false;
      for(size_t i=0;i<nb_services_;++i) {
        int index = (current_service_+i) % nb_services_;
        if(busy_[index] == false) {
          current_service_ = index;
          found_avail = true;
          break;
        }
      }
      if(found_avail == false) {
        std::cout << ">>>> ERROR:failed to find avail - trigger failsafe" << std::endl;
        current_service_ = rand()%nb_services_;
      }
      m.unlock();
      return ret;
    }

    void set_free(int i) {
      if(i>=0 && i<nb_services_) {
        m.lock();
        busy_[i] = false;
        int count = 1;
        for(size_t i=0;i<nb_services_;++i) {
          if(busy_[i] == false) {
            count++;
          }
        }
        std::cout << "server workers available: " << count << "/" << nb_services_ << std::endl;
        m.unlock();
      }
    }

private:
    void run(boost::asio::io_service&);

private:
    bool running_ = false;
    const size_t nb_thread_;
    const size_t nb_services_;
    std::string name_;
    std::mutex m;

    std::atomic_uint current_service_{0};
    std::atomic_uint running_threads_{0};

    std::vector<std::thread*> threads_;
    std::vector<std::shared_ptr<boost::asio::io_service>> services_;
    std::vector<bool> busy_;
    std::vector<std::unique_ptr<boost::asio::io_service::work>> works_;

};

template <typename Callable>
void ThreadPool::postAll(Callable callable)
{
    for (size_t i = 0; i < nb_thread_; ++i)
    {
        post(callable);
    }
}

template <typename Callable>
void ThreadPool::dispatchAll(Callable callable)
{
    for (size_t i = 0; i < nb_thread_; ++i)
    {
        post(callable);
    }
}

}
}

#endif

