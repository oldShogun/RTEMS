/**
 * @file thread_pool.hpp
 * @brief Пул потоков для асинхронного выполнения задач в системе RTEMS
 * @details Предоставляет функционал для создания и управления группой рабочих потоков,
 *          которые могут параллельно выполнять задачи
 * 
 * @author RTEMS Team
 * @date 2023
 */

#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <memory>
#include <atomic>

#include "common/logger.hpp"

namespace rtems {
namespace core {

/**
 * @brief Пул потоков для асинхронного выполнения задач
 * 
 * @details Класс создает и управляет группой рабочих потоков, которые ожидают задач из очереди.
 *          Каждый поток извлекает задачу из очереди, выполняет её и переходит к следующей.
 *          Реализует паттерн "Пул объектов" для потоков.
 */
class ThreadPool {
public:
    /**
     * @brief Конструктор пула потоков
     * 
     * @param numThreads Количество рабочих потоков (по умолчанию равно количеству аппаратных потоков)
     */
    explicit ThreadPool(size_t numThreads = std::thread::hardware_concurrency());
    
    /**
     * @brief Деструктор пула потоков
     * 
     * @details Останавливает все рабочие потоки
     */
    ~ThreadPool();
    
    /**
     * @brief Запрещаем копирование и перемещение
     */
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;
    
    /**
     * @brief Добавить задачу в очередь на выполнение
     * 
     * @tparam F Тип функции задачи
     * @tparam Args Типы аргументов функции
     * @param f Функция для выполнения
     * @param args Аргументы функции
     * @return std::future<typename std::result_of<F(Args...)>::type> Объект future для получения результата
     */
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type> {
        using return_type = typename std::result_of<F(Args...)>::type;
        
        // Создаем упакованную задачу
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<return_type> result = task->get_future();
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            
            // Проверяем, не остановлен ли пул
            if (m_stop) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            
            // Добавляем задачу в очередь
            m_tasks.emplace([task]() { (*task)(); });
        }
        
        // Уведомляем один из ожидающих потоков, что появилась новая задача
        m_condition.notify_one();
        return result;
    }
    
    /**
     * @brief Получить количество активных потоков в пуле
     * 
     * @return size_t Количество потоков
     */
    size_t size() const;
    
    /**
     * @brief Получить количество задач, ожидающих выполнения
     * 
     * @return size_t Количество задач в очереди
     */
    size_t queueSize() const;
    
    /**
     * @brief Получить количество активных задач (выполняемых в данный момент)
     * 
     * @return size_t Количество активных задач
     */
    size_t activeTaskCount() const;
    
    /**
     * @brief Остановить пул потоков (после выполнения всех задач)
     */
    void shutdown();
    
    /**
     * @brief Остановить пул потоков немедленно (задачи в очереди не будут выполнены)
     */
    void shutdownNow();
    
    /**
     * @brief Проверить, остановлен ли пул потоков
     * 
     * @return true Если пул остановлен
     * @return false Если пул работает
     */
    bool isStopped() const;
    
private:
    // Рабочие потоки
    std::vector<std::thread> m_workers;
    
    // Очередь задач
    std::queue<std::function<void()>> m_tasks;
    
    // Синхронизация
    mutable std::mutex m_queueMutex;
    std::condition_variable m_condition;
    
    // Флаг остановки пула
    std::atomic<bool> m_stop;
    
    // Счетчик активных задач
    std::atomic<size_t> m_activeTaskCount;
};

} // namespace core
} // namespace rtems 