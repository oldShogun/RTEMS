/**
 * @file task.hpp
 * @brief Определение классов задач и очереди задач в системе RTEMS
 * @details Предоставляет абстракцию асинхронных задач и их приоритетную очередь
 * 
 * @author RTEMS Team
 * @date 2023
 */

#pragma once

#include <string>
#include <functional>
#include <future>
#include <chrono>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>

namespace rtems {
namespace core {

/**
 * @brief Перечисление уровней приоритета задач
 */
enum class TaskPriority {
    LOW = 0,       ///< Низкий приоритет
    NORMAL = 1,    ///< Обычный приоритет
    HIGH = 2,      ///< Высокий приоритет
    CRITICAL = 3   ///< Критический приоритет
};

/**
 * @brief Состояние выполнения задачи
 */
enum class TaskState {
    CREATED,     ///< Задача создана, но не запущена
    QUEUED,      ///< Задача добавлена в очередь
    RUNNING,     ///< Задача в процессе выполнения
    COMPLETED,   ///< Задача успешно завершена
    FAILED,      ///< Задача завершилась с ошибкой
    CANCELLED    ///< Задача была отменена
};

/**
 * @brief Класс базовой задачи
 * 
 * @details Абстрактный класс, представляющий задачу, которая может быть выполнена асинхронно
 */
class ITask {
public:
    /**
     * @brief Виртуальный деструктор
     */
    virtual ~ITask() = default;
    
    /**
     * @brief Получить уникальный идентификатор задачи
     * 
     * @return size_t Идентификатор задачи
     */
    virtual size_t getTaskId() const = 0;
    
    /**
     * @brief Получить имя задачи
     * 
     * @return const std::string& Имя задачи
     */
    virtual const std::string& getName() const = 0;
    
    /**
     * @brief Получить приоритет задачи
     * 
     * @return TaskPriority Приоритет задачи
     */
    virtual TaskPriority getPriority() const = 0;
    
    /**
     * @brief Получить состояние задачи
     * 
     * @return TaskState Состояние задачи
     */
    virtual TaskState getState() const = 0;
    
    /**
     * @brief Выполнить задачу
     * 
     * @return true Если задача выполнена успешно
     * @return false Если произошла ошибка при выполнении
     */
    virtual bool execute() = 0;
    
    /**
     * @brief Отменить выполнение задачи
     * 
     * @return true Если задача успешно отменена
     * @return false Если задачу нельзя отменить
     */
    virtual bool cancel() = 0;
    
    /**
     * @brief Проверить, можно ли отменить задачу
     * 
     * @return true Если задачу можно отменить
     * @return false Если задачу нельзя отменить
     */
    virtual bool isCancellable() const = 0;
    
    /**
     * @brief Проверить, завершена ли задача
     * 
     * @return true Если задача завершена
     * @return false Если задача не завершена
     */
    virtual bool isCompleted() const = 0;
    
    /**
     * @brief Получить время создания задачи
     * 
     * @return std::chrono::system_clock::time_point Время создания
     */
    virtual std::chrono::system_clock::time_point getCreationTime() const = 0;
    
    /**
     * @brief Получить время начала выполнения задачи
     * 
     * @return std::chrono::system_clock::time_point Время начала выполнения
     */
    virtual std::chrono::system_clock::time_point getStartTime() const = 0;
    
    /**
     * @brief Получить время завершения задачи
     * 
     * @return std::chrono::system_clock::time_point Время завершения
     */
    virtual std::chrono::system_clock::time_point getEndTime() const = 0;
};

/**
 * @brief Шаблонный класс задачи
 * 
 * @tparam ReturnType Тип возвращаемого значения задачи
 */
template<typename ReturnType = void>
class Task : public ITask {
public:
    using TaskFunction = std::function<ReturnType()>;
    
    /**
     * @brief Конструктор задачи
     * 
     * @param name Имя задачи
     * @param function Функция для выполнения
     * @param priority Приоритет задачи
     */
    Task(const std::string& name, TaskFunction function, TaskPriority priority = TaskPriority::NORMAL)
        : m_name(name)
        , m_function(function)
        , m_priority(priority)
        , m_state(TaskState::CREATED)
        , m_cancellable(true)
        , m_creationTime(std::chrono::system_clock::now())
        , m_id(s_nextTaskId.fetch_add(1))
    {
    }
    
    /**
     * @brief Выполнить задачу
     * 
     * @return true Если задача выполнена успешно
     * @return false Если произошла ошибка при выполнении
     */
    bool execute() override {
        if (m_state == TaskState::RUNNING || m_state == TaskState::COMPLETED || 
            m_state == TaskState::FAILED || m_state == TaskState::CANCELLED) {
            return false;
        }
        
        m_state = TaskState::RUNNING;
        m_startTime = std::chrono::system_clock::now();
        
        try {
            m_promise = std::make_shared<std::promise<ReturnType>>();
            m_future = m_promise->get_future();
            
            if constexpr (std::is_void_v<ReturnType>) {
                m_function();
                m_promise->set_value();
            } else {
                m_promise->set_value(m_function());
            }
            
            m_state = TaskState::COMPLETED;
            m_endTime = std::chrono::system_clock::now();
            return true;
        } catch(const std::exception&) {
            m_state = TaskState::FAILED;
            m_endTime = std::chrono::system_clock::now();
            return false;
        }
    }
    
    /**
     * @brief Получить результат выполнения задачи
     * 
     * @return ReturnType Результат выполнения
     */
    ReturnType getResult() {
        if (m_future.valid()) {
            return m_future.get();
        }
        throw std::runtime_error("Task has no valid result");
    }
    
    /**
     * @brief Отменить выполнение задачи
     * 
     * @return true Если задача успешно отменена
     * @return false Если задачу нельзя отменить
     */
    bool cancel() override {
        if (!m_cancellable || m_state == TaskState::RUNNING || 
            m_state == TaskState::COMPLETED || m_state == TaskState::FAILED) {
            return false;
        }
        
        m_state = TaskState::CANCELLED;
        m_endTime = std::chrono::system_clock::now();
        return true;
    }
    
    /**
     * @brief Проверить, можно ли отменить задачу
     * 
     * @return true Если задачу можно отменить
     * @return false Если задачу нельзя отменить
     */
    bool isCancellable() const override {
        return m_cancellable;
    }
    
    /**
     * @brief Установить возможность отмены задачи
     * 
     * @param cancellable Флаг возможности отмены
     */
    void setCancellable(bool cancellable) {
        m_cancellable = cancellable;
    }
    
    /**
     * @brief Проверить, завершена ли задача
     * 
     * @return true Если задача завершена
     * @return false Если задача не завершена
     */
    bool isCompleted() const override {
        return m_state == TaskState::COMPLETED;
    }
    
    /**
     * @brief Получить уникальный идентификатор задачи
     * 
     * @return size_t Идентификатор задачи
     */
    size_t getTaskId() const override {
        return m_id;
    }
    
    /**
     * @brief Получить имя задачи
     * 
     * @return const std::string& Имя задачи
     */
    const std::string& getName() const override {
        return m_name;
    }
    
    /**
     * @brief Получить приоритет задачи
     * 
     * @return TaskPriority Приоритет задачи
     */
    TaskPriority getPriority() const override {
        return m_priority;
    }
    
    /**
     * @brief Получить состояние задачи
     * 
     * @return TaskState Состояние задачи
     */
    TaskState getState() const override {
        return m_state;
    }
    
    /**
     * @brief Получить время создания задачи
     * 
     * @return std::chrono::system_clock::time_point Время создания
     */
    std::chrono::system_clock::time_point getCreationTime() const override {
        return m_creationTime;
    }
    
    /**
     * @brief Получить время начала выполнения задачи
     * 
     * @return std::chrono::system_clock::time_point Время начала выполнения
     */
    std::chrono::system_clock::time_point getStartTime() const override {
        return m_startTime;
    }
    
    /**
     * @brief Получить время завершения задачи
     * 
     * @return std::chrono::system_clock::time_point Время завершения
     */
    std::chrono::system_clock::time_point getEndTime() const override {
        return m_endTime;
    }
    
private:
    std::string m_name;
    TaskFunction m_function;
    TaskPriority m_priority;
    std::atomic<TaskState> m_state;
    bool m_cancellable;
    
    std::chrono::system_clock::time_point m_creationTime;
    std::chrono::system_clock::time_point m_startTime;
    std::chrono::system_clock::time_point m_endTime;
    
    size_t m_id;
    std::shared_ptr<std::promise<ReturnType>> m_promise;
    std::future<ReturnType> m_future;
    
    static std::atomic<size_t> s_nextTaskId;
};

// Инициализация статического счетчика ID задач
template<typename ReturnType>
std::atomic<size_t> Task<ReturnType>::s_nextTaskId(1);

/**
 * @brief Компаратор для сравнения указателей на задачи по приоритету
 */
struct TaskPriorityComparator {
    bool operator()(const std::shared_ptr<ITask>& a, const std::shared_ptr<ITask>& b) const {
        return static_cast<int>(a->getPriority()) < static_cast<int>(b->getPriority());
    }
};

/**
 * @brief Очередь задач с приоритетами
 */
class TaskQueue {
public:
    /**
     * @brief Конструктор очереди задач
     */
    TaskQueue() = default;
    
    /**
     * @brief Деструктор очереди задач
     */
    ~TaskQueue() = default;
    
    /**
     * @brief Добавить задачу в очередь
     * 
     * @param task Задача для добавления
     */
    void push(std::shared_ptr<ITask> task) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.push(task);
        m_condition.notify_one();
    }
    
    /**
     * @brief Извлечь задачу из очереди
     * 
     * @param timeout Время ожидания
     * @return std::shared_ptr<ITask> Указатель на задачу или nullptr, если очередь пуста
     */
    std::shared_ptr<ITask> pop(const std::chrono::milliseconds& timeout = std::chrono::milliseconds(0)) {
        std::unique_lock<std::mutex> lock(m_mutex);
        
        if (timeout.count() > 0) {
            m_condition.wait_for(lock, timeout, [this] { return !m_queue.empty() || m_stop; });
        }
        
        if (m_queue.empty() || m_stop) {
            return nullptr;
        }
        
        auto task = m_queue.top();
        m_queue.pop();
        return task;
    }
    
    /**
     * @brief Проверить, пуста ли очередь
     * 
     * @return true Если очередь пуста
     * @return false Если в очереди есть задачи
     */
    bool empty() const {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }
    
    /**
     * @brief Получить размер очереди
     * 
     * @return size_t Количество задач в очереди
     */
    size_t size() const {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.size();
    }
    
    /**
     * @brief Остановить очередь
     */
    void stop() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_stop = true;
        m_condition.notify_all();
    }
    
    /**
     * @brief Проверить, остановлена ли очередь
     * 
     * @return true Если очередь остановлена
     * @return false Если очередь работает
     */
    bool isStopped() const {
        return m_stop;
    }
    
private:
    std::priority_queue<
        std::shared_ptr<ITask>,
        std::vector<std::shared_ptr<ITask>>,
        TaskPriorityComparator
    > m_queue;
    
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    bool m_stop = false;
};

} // namespace core
} // namespace rtems 