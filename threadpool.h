#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <atomic>		// std::atomic
#include <condition_variable>	// std::condition_variable
#include <functional>		// std::function
#include <iostream>		// std::cout
#include <mutex>		// std::mutex, std::unique_lock
#include <thread>		// std::thread
#include <queue>                // std::queue


class ThreadPool
{
    class WorkerThread
    {
    public:
        enum STATE : unsigned char{
            RUNNING = 0,
            DESTROY,
            COMPLETE_ALL_TASK_AND_DESTROY
        };
    private:
        ThreadPool* _pool;
        std::thread* _thread;
        std::atomic<STATE> _state;
        bool _should_wakeup()
        {
            return  ((!_pool->_task_queue.empty()) || (_state != STATE::RUNNING));
        }

    public:
        WorkerThread(ThreadPool* pool) : _pool(pool), _state(STATE::RUNNING)
        {
            _thread = new std::thread([this](){
                for(;;)
                {
                    std::function<void()> task = nullptr;
                    {
                        std::unique_lock<std::mutex> lock(_pool->_task_queue_lock);
                        while(!_should_wakeup())
                            _pool->_condition.wait(lock);
                        if(this->_state == STATE::DESTROY)
                        {
                            return;
                        }
                        if((this->_state == STATE::COMPLETE_ALL_TASK_AND_DESTROY) && (_pool->_task_queue.empty()))
                        {
                            return;
                        }
                        if(!_pool->_task_queue.empty())
                        {
                            task = std::move(_pool->_task_queue.front());
                            _pool->_task_queue.pop();
                        }
                    }
                    if(task != nullptr)
                        task();
                }
            });
        }

        ~WorkerThread()
        {
            _pool->_condition.notify_all();
            _thread->join();
        }

        STATE state()
        {
            return _state.load();
        }

        void state(STATE s)
        {
            _state = s;
        }
    };
private:
    std::queue < std::unique_ptr< WorkerThread > > _worker_queue;
    std::mutex _worker_queue_lock;

    std::queue < std::function<void()> > _task_queue;
    std::mutex _task_queue_lock;
    std::condition_variable _condition;
public:
    ThreadPool()
    {
        std::unique_lock<std::mutex> lock(_worker_queue_lock);
        for(unsigned i = 0 ; i < std::thread::hardware_concurrency() ; i++)
        {
            try
            {
                _worker_queue.emplace(new WorkerThread(this));
            }
            catch(const std::bad_alloc &ex)
            {
                std::cout<<ex.what()<<std::endl;
                break;
            }
        }
        std::cout<<_worker_queue.size()<<" threads\n";
    }

    ThreadPool(size_t size)
    {
        std::unique_lock<std::mutex> lock(_worker_queue_lock);
        for(size_t i = 0 ; i < size ; i++)
        {
            try
            {
                _worker_queue.emplace(new WorkerThread(this));
            }
            catch(const std::bad_alloc &ex)
            {
                std::cout<<ex.what()<<std::endl;
                break;
            }
        }
        std::cout<<_worker_queue.size()<<" threads\n";
    }

    ~ThreadPool()
    {
        destroy();
    }

    void resize(size_t size)
    {
        if(size == 0)
        {
            return;
        }
        std::unique_lock<std::mutex> lock(_worker_queue_lock);
        if(size < _worker_queue.size())
        {
            while(size < _worker_queue.size())
            {
                _worker_queue.front()->state(WorkerThread::DESTROY);
                _worker_queue.pop();
            }
        }
        else if(size > _worker_queue.size())
        {
            while(size > _worker_queue.size())
            {
                try
                {
                    _worker_queue.emplace(new WorkerThread(this));
                }
                catch(const std::bad_alloc &ex)
                {
                    std::cout<<ex.what()<<std::endl;
                    break;
                }
            }
        }
        std::cout<<_worker_queue.size()<<" threads\n";
    }

    void destroy()
    {
        std::unique_lock<std::mutex> lock(_worker_queue_lock);
        while(!_worker_queue.empty())
        {
            _worker_queue.front()->state(WorkerThread::COMPLETE_ALL_TASK_AND_DESTROY);
            _worker_queue.pop();
        }
    }

    void enqueue(std::function<void()> task)
    {
        {
            std::unique_lock<std::mutex> lock(_worker_queue_lock);
            if((_worker_queue.empty() == true) || (_worker_queue.front()->state() != WorkerThread::RUNNING))
            {
                return;
            }
        }
        {
            std::unique_lock<std::mutex> lock(_task_queue_lock);
            _task_queue.push(task);
        }
        _condition.notify_one();
    }

    size_t tasks()
    {
        std::unique_lock<std::mutex> lock(_task_queue_lock);
        return _task_queue.size();
    }
};

#endif
