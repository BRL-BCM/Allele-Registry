/*
 * File: /home/thomas/perso/commonpp/src/commonpp/thread/ThreadPool.cpp
 * Part of commonpp.
 *
 * Distributed under the 2-clause BSD licence (See LICENCE.TXT file at the
 * project root).
 *
 * Copyright (c) 2015 Thomas Sanchez.  All rights reserved.
 *
 */
#include "httpp/utils/ThreadPool.hpp"

#include <atomic>
#include <functional>
#include <iostream>

#include <boost/log/trivial.hpp>

namespace HTTPP
{
namespace UTILS
{

ThreadPool::ThreadPool(size_t nb_thread, std::string name, size_t nb_services)
: nb_thread_(nb_thread)
, nb_services_(nb_services)
, name_(std::move(name))
{
    if (nb_services < 1)
    {
        throw std::invalid_argument("nb_services must be > 0");
    }

    if (nb_services > nb_thread_)
    {
        throw std::invalid_argument("not enough thread configured, at least 1 "
                                    "thread by service is required");
    }

    if (nb_thread_ % nb_services != 0)
    {
        throw std::invalid_argument(
            "All the services should have the same number of thread");
    }

    services_.reserve(nb_services);
    works_.reserve(nb_services);
    const std::size_t concurrency_hint = nb_thread / nb_services;
    std::generate_n(std::back_inserter(services_), nb_services, [concurrency_hint] {
        return std::make_shared<boost::asio::io_service>(concurrency_hint);
    });
}

ThreadPool::~ThreadPool()
{
    if (running_)
    {
        stop();
    }
}


void ThreadPool::start(ThreadInit fct)
{
    if (running_)
    {
        return;
    }

    int i = 0;
    std::generate_n(std::back_inserter(works_), nb_services_, [this, &i] {
        return std::unique_ptr<boost::asio::io_service::work>(
            new boost::asio::io_service::work(*services_[i++]));
    });
    busy_.reserve(nb_services_);
    for (size_t i = 0; i < nb_services_; ++i) busy_[i] = false;
    threads_.reserve(nb_thread_);
    for (size_t i = 0; i < nb_thread_; ++i)
    {
        auto ref = std::ref(getService(i % nb_services_));
        auto worker = new std::thread(&ThreadPool::run, this, ref);
        threads_[i] = worker;

    }

    while (running_threads_ != nb_thread_)
    {
        std::this_thread::yield();
    }

    running_ = true;
}

void ThreadPool::run(boost::asio::io_service& service)
{
    m.lock();
    BOOST_LOG_TRIVIAL(debug) << "start thread";
    m.unlock();
    ++running_threads_;
    m.lock();
    BOOST_LOG_TRIVIAL(debug) << "call run()";
    m.unlock();
    service.run();
    m.lock();
    BOOST_LOG_TRIVIAL(debug) << "is stopping";
    m.unlock();
    --running_threads_;

}


void ThreadPool::stop()
{
    if (!running_)
    {
        return;
    }

    running_ = false;
    works_.clear();

    for (auto& service : services_)
    {
        service->stop();
    }

    for (auto& th : threads_)
    {
        if (th->joinable())
        {
            th->join();
            delete th;
        }
    }

    threads_.clear();

    for (auto& service : services_)
    {
        service->reset();
    }
}

boost::asio::io_service& ThreadPool::getService(int service_id)
{
    if (nb_services_ == 1)
    {
        return *services_.front();
    }

    int i, index;
    switch (service_id)
    {
    case ROUND_ROBIN:
        return *services_[current_service_++ % nb_services_];
    default:
        return *services_[service_id];
    }
}

bool ThreadPool::runningInPool() const noexcept
{
    auto current_id = std::this_thread::get_id();
    for (const auto& thread : threads_)
    {
        if (thread->get_id() == current_id)
        {
            return true;
        }
    }

    return false;
}

size_t ThreadPool::threads() const noexcept
{
    return nb_thread_;
}


}
}
