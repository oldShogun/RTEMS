/**
 * @file thread_pool.cpp
 * @brief Реализация пула потоков для асинхронного выполнения задач
 * 
 * @author RTEMS Team
 * @date 2023
 */

#include "core/thread_pool.hpp"
#include "common/logger.hpp"

namespace rtems {
namespace core {

ThreadPool::ThreadPool(size_t numThreads)
    : m_stop(false)
    , m_activeTaskCount(0)
{
    for (size_t i = 0; i < numThreads; ++i) {
        m_workers.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                
                {
                    std::unique_lock<std::mutex> lock(this->m_queueMutex);
                    
                    // Ожидаем, пока не появится задача или не будет подан сигнал остановки
                    this->m_condition.wait(lock, [this] {
                        return this->m_stop || !this->m_tasks.empty();
                    });
                    
                    // Если пул остановлен и нет задач, выходим из цикла
                    if (this->m_stop && this->m_tasks.empty()) {
                        return;
                    }
                    
                    // Получаем задачу из очереди
                    task = std::move(this->m_tasks.front());
                    this->m_tasks.pop();
                }
                
                // Увеличиваем счетчик активных задач
                ++m_activeTaskCount;
                
                // Выполняем задачу
                try {
                    task();
                } catch (const std::exception& e) {
                    RTEMS_LOG_ERROR("Exception in thread pool task: {}", e.what());
                } catch (...) {
                    RTEMS_LOG_ERROR("Unknown exception in thread pool task");
                }
                
                // Уменьшаем счетчик активных задач
                --m_activeTaskCount;
            }
        });
    }
    
    RTEMS_LOG_INFO("Thread pool created with {} worker threads", numThreads);
}

ThreadPool::~ThreadPool() {
    shutdownNow();
}

size_t ThreadPool::size() const {
    return m_workers.size();
}

size_t ThreadPool::queueSize() const {
    std::unique_lock<std::mutex> lock(m_queueMutex);
    return m_tasks.size();
}

size_t ThreadPool::activeTaskCount() const {
    return m_activeTaskCount;
}

void ThreadPool::shutdown() {
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_stop = true;
    }
    
    // Уведомляем все потоки, чтобы они могли завершить работу
    m_condition.notify_all();
    
    // Ожидаем завершения всех потоков
    for (std::thread& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    RTEMS_LOG_INFO("Thread pool shut down gracefully");
}

void ThreadPool::shutdownNow() {
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_stop = true;
        
        // Очищаем очередь задач
        while (!m_tasks.empty()) {
            m_tasks.pop();
        }
    }
    
    // Уведомляем все потоки, чтобы они могли завершить работу
    m_condition.notify_all();
    
    // Ожидаем завершения всех потоков
    for (std::thread& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    RTEMS_LOG_INFO("Thread pool shut down immediately");
}

bool ThreadPool::isStopped() const {
    return m_stop;
}

} // namespace core
} // namespace rtems 