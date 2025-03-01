/**
 * @file task_scheduler.cpp
 * @brief Реализация планировщика задач для системы RTEMS
 * 
 * @author RTEMS Team
 * @date 2023
 */

#include "core/task_scheduler.hpp"
#include "common/logger.hpp"
#include <sstream>

namespace rtems {
namespace core {

TaskScheduler& TaskScheduler::getInstance() {
    static TaskScheduler instance;
    return instance;
}

TaskScheduler::TaskScheduler()
    : m_running(false)
{
    LOG_INFO("TaskScheduler created");
}

TaskScheduler::~TaskScheduler() {
    stop();
    LOG_INFO("TaskScheduler destroyed");
}

void TaskScheduler::start(size_t threadCount) {
    if (m_running) {
        LOG_WARNING("TaskScheduler is already running");
        return;
    }
    
    std::ostringstream oss;
    oss << "Starting TaskScheduler with " << threadCount << " threads";
    LOG_INFO(oss.str());
    
    m_threadPool = std::make_unique<ThreadPool>(threadCount);
    m_running = true;
    
    // Запускаем поток обработки периодических задач
    m_periodicTaskThread = std::thread(&TaskScheduler::periodicTaskWorker, this);
    
    // Запускаем поток обработки запланированных задач
    m_scheduledTaskThread = std::thread(&TaskScheduler::scheduledTaskWorker, this);
    
    LOG_INFO("TaskScheduler started");
}

void TaskScheduler::stop() {
    if (!m_running) {
        return;
    }
    
    LOG_INFO("Stopping TaskScheduler");
    
    m_running = false;
    
    // Ожидаем завершения потока обработки периодических задач
    if (m_periodicTaskThread.joinable()) {
        m_periodicTaskThread.join();
    }
    
    // Ожидаем завершения потока обработки запланированных задач
    if (m_scheduledTaskThread.joinable()) {
        m_scheduledTaskThread.join();
    }
    
    // Останавливаем пул потоков
    if (m_threadPool) {
        m_threadPool->shutdown();
    }
    
    LOG_INFO("TaskScheduler stopped");
}

bool TaskScheduler::cancelPeriodicTask(size_t taskId) {
    std::unique_lock<std::mutex> lock(m_periodicTaskMutex);
    
    auto it = m_periodicTasks.find(taskId);
    if (it == m_periodicTasks.end()) {
        return false;
    }
    
    it->second.active = false;
    return it->second.task->cancel();
}

bool TaskScheduler::cancelScheduledTask(size_t taskId) {
    std::unique_lock<std::mutex> lock(m_scheduledTaskMutex);
    
    auto it = m_scheduledTasks.find(taskId);
    if (it == m_scheduledTasks.end()) {
        return false;
    }
    
    bool result = it->second.task->cancel();
    m_scheduledTasks.erase(it);
    return result;
}

bool TaskScheduler::isRunning() const {
    return m_running;
}

size_t TaskScheduler::getPeriodicTaskCount() const {
    std::unique_lock<std::mutex> lock(m_periodicTaskMutex);
    return m_periodicTasks.size();
}

size_t TaskScheduler::getScheduledTaskCount() const {
    std::unique_lock<std::mutex> lock(m_scheduledTaskMutex);
    return m_scheduledTasks.size();
}

void TaskScheduler::periodicTaskWorker() {
    LOG_INFO("Periodic task worker started");
    
    while (m_running) {
        auto now = std::chrono::steady_clock::now();
        std::vector<std::shared_ptr<ITask>> tasksToExecute;
        
        {
            std::unique_lock<std::mutex> lock(m_periodicTaskMutex);
            
            for (auto& taskPair : m_periodicTasks) {
                auto& taskInfo = taskPair.second;
                
                if (!taskInfo.active) {
                    continue;
                }
                
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - taskInfo.lastExecution).count();
                
                if (elapsed >= taskInfo.intervalMs) {
                    tasksToExecute.push_back(taskInfo.task);
                    taskInfo.lastExecution = now;
                }
            }
        }
        
        // Выполняем задачи, чьи интервалы истекли
        for (auto& task : tasksToExecute) {
            m_threadPool->enqueue([task]() {
                try {
                    task->execute();
                } catch (const std::exception& e) {
                    std::ostringstream oss;
                    oss << "Exception in periodic task " << task->getName() << ": " << e.what();
                    LOG_ERROR(oss.str());
                } catch (...) {
                    std::ostringstream oss;
                    oss << "Unknown exception in periodic task " << task->getName();
                    LOG_ERROR(oss.str());
                }
            });
        }
        
        // Спим 1 миллисекунду для проверки задач с минимальным интервалом
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    LOG_INFO("Periodic task worker stopped");
}

void TaskScheduler::scheduledTaskWorker() {
    LOG_INFO("Scheduled task worker started");
    
    while (m_running) {
        auto now = std::chrono::system_clock::now();
        std::vector<std::shared_ptr<ITask>> tasksToExecute;
        std::vector<size_t> tasksToRemove;
        
        {
            std::unique_lock<std::mutex> lock(m_scheduledTaskMutex);
            
            for (auto& taskPair : m_scheduledTasks) {
                auto& taskInfo = taskPair.second;
                
                if (now >= taskInfo.executeAt) {
                    tasksToExecute.push_back(taskInfo.task);
                    tasksToRemove.push_back(taskPair.first);
                }
            }
            
            // Удаляем выполненные задачи из списка запланированных
            for (auto taskId : tasksToRemove) {
                m_scheduledTasks.erase(taskId);
            }
        }
        
        // Выполняем задачи, время которых наступило
        for (auto& task : tasksToExecute) {
            m_threadPool->enqueue([task]() {
                try {
                    task->execute();
                } catch (const std::exception& e) {
                    std::ostringstream oss;
                    oss << "Exception in scheduled task " << task->getName() << ": " << e.what();
                    LOG_ERROR(oss.str());
                } catch (...) {
                    std::ostringstream oss;
                    oss << "Unknown exception in scheduled task " << task->getName();
                    LOG_ERROR(oss.str());
                }
            });
        }
        
        // Спим 100 миллисекунд перед следующей проверкой запланированных задач
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    LOG_INFO("Scheduled task worker stopped");
}

} // namespace core
} // namespace rtems 