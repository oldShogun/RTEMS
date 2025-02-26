/**
 * @file task_scheduler.hpp
 * @brief Планировщик задач для системы RTEMS
 * @details Предоставляет функционал для управления выполнением задач в системе
 * 
 * @author RTEMS Team
 * @date 2023
 */

#pragma once

#include <memory>
#include <map>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <future>
#include <chrono>

#include "core/thread_pool.hpp"
#include "core/task.hpp"
#include "common/logger.hpp"

namespace rtems {
namespace core {

/**
 * @brief Периодические интервалы для выполнения задач
 */
enum class TaskInterval {
    ONCE = 0,          ///< Выполнить один раз
    MILLISECOND = 1,   ///< Выполнять каждую миллисекунду
    CENTISECOND = 10,  ///< Выполнять каждые 10 миллисекунд
    DECISECOND = 100,  ///< Выполнять каждые 100 миллисекунд
    SECOND = 1000,     ///< Выполнять каждую секунду
    MINUTE = 60000,    ///< Выполнять каждую минуту
    HOUR = 3600000     ///< Выполнять каждый час
};

/**
 * @brief Планировщик задач
 * 
 * @details Класс, управляющий выполнением задач в системе RTEMS.
 *          Позволяет запланировать задачи на определенное время, с определенной периодичностью
 *          или при наступлении определенных событий.
 */
class TaskScheduler {
public:
    /**
     * @brief Получить экземпляр планировщика задач (Singleton)
     * 
     * @return TaskScheduler& Ссылка на экземпляр планировщика
     */
    static TaskScheduler& getInstance();
    
    /**
     * @brief Запустить планировщик
     * 
     * @param threadCount Количество потоков в пуле (по умолчанию количество доступных аппаратных потоков)
     */
    void start(size_t threadCount = std::thread::hardware_concurrency());
    
    /**
     * @brief Остановить планировщик
     */
    void stop();
    
    /**
     * @brief Запланировать выполнение задачи
     * 
     * @tparam F Тип функции
     * @tparam Args Типы аргументов функции
     * @param name Имя задачи
     * @param priority Приоритет задачи
     * @param func Функция для выполнения
     * @param args Аргументы функции
     * @return std::future<typename std::result_of<F(Args...)>::type> Объект future для получения результата
     */
    template<typename F, typename... Args>
    auto schedule(const std::string& name, TaskPriority priority, F&& func, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type> {
        using ReturnType = typename std::result_of<F(Args...)>::type;
        
        auto boundTask = std::bind(std::forward<F>(func), std::forward<Args>(args)...);
        auto task = std::make_shared<Task<ReturnType>>(name, boundTask, priority);
        
        std::packaged_task<ReturnType()> packagedTask([task]() {
            task->execute();
            return task->getResult();
        });
        
        std::future<ReturnType> result = packagedTask.get_future();
        
        {
            std::unique_lock<std::mutex> lock(m_taskMutex);
            m_taskQueue.push(task);
        }
        
        m_threadPool->enqueue(std::move(packagedTask));
        
        return result;
    }
    
    /**
     * @brief Запланировать периодическое выполнение задачи
     * 
     * @tparam F Тип функции
     * @tparam Args Типы аргументов функции
     * @param name Имя задачи
     * @param interval Интервал выполнения
     * @param priority Приоритет задачи
     * @param func Функция для выполнения
     * @param args Аргументы функции
     * @return size_t Идентификатор периодической задачи
     */
    template<typename F, typename... Args>
    size_t scheduleInterval(const std::string& name, TaskInterval interval, TaskPriority priority, 
                          F&& func, Args&&... args) {
        using ReturnType = typename std::result_of<F(Args...)>::type;
        
        auto boundTask = std::bind(std::forward<F>(func), std::forward<Args>(args)...);
        auto task = std::make_shared<Task<ReturnType>>(name, boundTask, priority);
        
        size_t taskId = task->getTaskId();
        
        std::unique_lock<std::mutex> lock(m_periodicTaskMutex);
        
        m_periodicTasks[taskId] = TaskInfo {
            task,
            static_cast<int>(interval),
            std::chrono::steady_clock::now(),
            true
        };
        
        return taskId;
    }
    
    /**
     * @brief Запланировать выполнение задачи в определенное время
     * 
     * @tparam F Тип функции
     * @tparam Args Типы аргументов функции
     * @param name Имя задачи
     * @param executeAt Время выполнения
     * @param priority Приоритет задачи
     * @param func Функция для выполнения
     * @param args Аргументы функции
     * @return size_t Идентификатор запланированной задачи
     */
    template<typename F, typename... Args>
    size_t scheduleAt(const std::string& name, std::chrono::system_clock::time_point executeAt, 
                    TaskPriority priority, F&& func, Args&&... args) {
        using ReturnType = typename std::result_of<F(Args...)>::type;
        
        auto boundTask = std::bind(std::forward<F>(func), std::forward<Args>(args)...);
        auto task = std::make_shared<Task<ReturnType>>(name, boundTask, priority);
        
        size_t taskId = task->getTaskId();
        
        std::unique_lock<std::mutex> lock(m_scheduledTaskMutex);
        
        m_scheduledTasks[taskId] = ScheduledTaskInfo {
            task,
            executeAt
        };
        
        return taskId;
    }
    
    /**
     * @brief Отменить периодическую задачу
     * 
     * @param taskId Идентификатор задачи
     * @return true Если задача успешно отменена
     * @return false Если задача не найдена или не может быть отменена
     */
    bool cancelPeriodicTask(size_t taskId);
    
    /**
     * @brief Отменить запланированную задачу
     * 
     * @param taskId Идентификатор задачи
     * @return true Если задача успешно отменена
     * @return false Если задача не найдена или не может быть отменена
     */
    bool cancelScheduledTask(size_t taskId);
    
    /**
     * @brief Проверить, запущен ли планировщик
     * 
     * @return true Если планировщик запущен
     * @return false Если планировщик остановлен
     */
    bool isRunning() const;
    
    /**
     * @brief Получить количество активных периодических задач
     * 
     * @return size_t Количество задач
     */
    size_t getPeriodicTaskCount() const;
    
    /**
     * @brief Получить количество запланированных задач
     * 
     * @return size_t Количество задач
     */
    size_t getScheduledTaskCount() const;
    
private:
    /**
     * @brief Структура для хранения информации о периодической задаче
     */
    struct TaskInfo {
        std::shared_ptr<ITask> task;
        int intervalMs;
        std::chrono::steady_clock::time_point lastExecution;
        bool active;
    };
    
    /**
     * @brief Структура для хранения информации о запланированной задаче
     */
    struct ScheduledTaskInfo {
        std::shared_ptr<ITask> task;
        std::chrono::system_clock::time_point executeAt;
    };
    
    /**
     * @brief Приватный конструктор (Singleton)
     */
    TaskScheduler();
    
    /**
     * @brief Приватный деструктор (Singleton)
     */
    ~TaskScheduler();
    
    /**
     * @brief Запретить копирование и перемещение
     */
    TaskScheduler(const TaskScheduler&) = delete;
    TaskScheduler& operator=(const TaskScheduler&) = delete;
    TaskScheduler(TaskScheduler&&) = delete;
    TaskScheduler& operator=(TaskScheduler&&) = delete;
    
    /**
     * @brief Поток обработки периодических задач
     */
    void periodicTaskWorker();
    
    /**
     * @brief Поток обработки запланированных задач
     */
    void scheduledTaskWorker();
    
    std::unique_ptr<ThreadPool> m_threadPool;
    TaskQueue m_taskQueue;
    
    std::unordered_map<size_t, TaskInfo> m_periodicTasks;
    std::unordered_map<size_t, ScheduledTaskInfo> m_scheduledTasks;
    
    mutable std::mutex m_taskMutex;
    mutable std::mutex m_periodicTaskMutex;
    mutable std::mutex m_scheduledTaskMutex;
    
    std::atomic<bool> m_running;
    std::thread m_periodicTaskThread;
    std::thread m_scheduledTaskThread;
};

} // namespace core
} // namespace rtems 