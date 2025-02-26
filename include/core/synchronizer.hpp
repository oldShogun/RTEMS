/**
 * @file synchronizer.hpp
 * @brief Синхронизатор для компонентов системы RTEMS
 * @details Предоставляет примитивы синхронизации для обеспечения безопасного доступа
 *          к разделяемым ресурсам между компонентами
 * 
 * @author RTEMS Team
 * @date 2023
 */

#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <memory>
#include <functional>

#include "common/logger.hpp"

namespace rtems {
namespace core {

/**
 * @brief Возможные результаты операций синхронизации
 */
enum class SyncResult {
    SUCCESS,        ///< Операция выполнена успешно
    TIMEOUT,        ///< Превышено время ожидания
    ALREADY_LOCKED, ///< Ресурс уже заблокирован
    NOT_LOCKED,     ///< Ресурс не заблокирован
    ERROR           ///< Ошибка синхронизации
};

/**
 * @brief Тип блокировки ресурса
 */
enum class LockType {
    SHARED,     ///< Разделяемая блокировка (чтение)
    EXCLUSIVE   ///< Эксклюзивная блокировка (запись)
};

/**
 * @brief Синхронизатор компонентов
 * 
 * @details Предоставляет централизованный механизм синхронизации доступа к разделяемым ресурсам
 *          между различными компонентами системы. Реализует паттерн Singleton.
 */
class Synchronizer {
public:
    /**
     * @brief Получить экземпляр синхронизатора (Singleton)
     * 
     * @return Synchronizer& Ссылка на экземпляр синхронизатора
     */
    static Synchronizer& getInstance();
    
    /**
     * @brief Зарегистрировать новый синхронизируемый ресурс
     * 
     * @param resourceId Идентификатор ресурса
     * @return true Если регистрация прошла успешно
     * @return false Если ресурс уже зарегистрирован
     */
    bool registerResource(const std::string& resourceId);
    
    /**
     * @brief Удалить зарегистрированный ресурс
     * 
     * @param resourceId Идентификатор ресурса
     * @return true Если удаление прошло успешно
     * @return false Если ресурс не зарегистрирован
     */
    bool unregisterResource(const std::string& resourceId);
    
    /**
     * @brief Заблокировать ресурс для доступа
     * 
     * @param resourceId Идентификатор ресурса
     * @param lockType Тип блокировки (разделяемая или эксклюзивная)
     * @param timeout Время ожидания в миллисекундах (0 - не ждать, -1 - ждать бесконечно)
     * @return SyncResult Результат операции
     */
    SyncResult lock(const std::string& resourceId, LockType lockType, int timeout = -1);
    
    /**
     * @brief Разблокировать ресурс
     * 
     * @param resourceId Идентификатор ресурса
     * @param lockType Тип снимаемой блокировки
     * @return SyncResult Результат операции
     */
    SyncResult unlock(const std::string& resourceId, LockType lockType);
    
    /**
     * @brief Проверить, заблокирован ли ресурс
     * 
     * @param resourceId Идентификатор ресурса
     * @return true Если ресурс заблокирован
     * @return false Если ресурс не заблокирован
     */
    bool isLocked(const std::string& resourceId) const;
    
    /**
     * @brief Выполнить операцию в рамках блокировки
     * 
     * @tparam F Тип функции
     * @tparam Args Типы аргументов функции
     * @param resourceId Идентификатор ресурса
     * @param lockType Тип блокировки
     * @param func Функция для выполнения
     * @param args Аргументы функции
     * @return auto Результат выполнения функции
     */
    template<typename F, typename... Args>
    auto withLock(const std::string& resourceId, LockType lockType, F&& func, Args&&... args) {
        auto result = lock(resourceId, lockType);
        if (result != SyncResult::SUCCESS) {
            throw std::runtime_error("Failed to lock resource: " + resourceId);
        }
        
        try {
            auto funcResult = std::invoke(std::forward<F>(func), std::forward<Args>(args)...);
            unlock(resourceId, lockType);
            return funcResult;
        } catch (...) {
            unlock(resourceId, lockType);
            throw;
        }
    }
    
    /**
     * @brief Создать объект блокировки ресурса (RAII)
     * 
     * @param resourceId Идентификатор ресурса
     * @param lockType Тип блокировки
     * @param timeout Время ожидания в миллисекундах
     * @return std::unique_ptr<ResourceLock> Объект блокировки
     */
    std::unique_ptr<class ResourceLock> createLock(
        const std::string& resourceId, 
        LockType lockType, 
        int timeout = -1);
    
    /**
     * @brief Ожидать условие на ресурсе
     * 
     * @param resourceId Идентификатор ресурса
     * @param predicate Предикат условия
     * @param timeout Время ожидания в миллисекундах
     * @return SyncResult Результат операции
     */
    SyncResult waitForCondition(
        const std::string& resourceId, 
        std::function<bool()> predicate, 
        int timeout = -1);
    
    /**
     * @brief Уведомить ожидающий поток о изменении условия
     * 
     * @param resourceId Идентификатор ресурса
     * @return SyncResult Результат операции
     */
    SyncResult notifyOne(const std::string& resourceId);
    
    /**
     * @brief Уведомить все ожидающие потоки о изменении условия
     * 
     * @param resourceId Идентификатор ресурса
     * @return SyncResult Результат операции
     */
    SyncResult notifyAll(const std::string& resourceId);
    
    /**
     * @brief Получить количество зарегистрированных ресурсов
     * 
     * @return size_t Количество ресурсов
     */
    size_t getResourceCount() const;
    
private:
    /**
     * @brief Структура ресурса синхронизации
     */
    struct SyncResource {
        std::shared_mutex mutex;
        std::condition_variable_any condition;
        std::atomic<size_t> sharedLocks{0};
        std::atomic<bool> exclusiveLocked{false};
    };
    
    /**
     * @brief Приватный конструктор (Singleton)
     */
    Synchronizer();
    
    /**
     * @brief Приватный деструктор (Singleton)
     */
    ~Synchronizer();
    
    /**
     * @brief Запретить копирование и перемещение
     */
    Synchronizer(const Synchronizer&) = delete;
    Synchronizer& operator=(const Synchronizer&) = delete;
    Synchronizer(Synchronizer&&) = delete;
    Synchronizer& operator=(Synchronizer&&) = delete;
    
    std::unordered_map<std::string, std::unique_ptr<SyncResource>> m_resources;
    mutable std::mutex m_resourceMutex;
};

/**
 * @brief Класс для автоматической блокировки/разблокировки ресурса (RAII)
 */
class ResourceLock {
public:
    /**
     * @brief Конструктор блокировки ресурса
     * 
     * @param synchronizer Ссылка на синхронизатор
     * @param resourceId Идентификатор ресурса
     * @param lockType Тип блокировки
     * @param result Результат операции блокировки
     */
    ResourceLock(Synchronizer& synchronizer, 
                 const std::string& resourceId, 
                 LockType lockType,
                 SyncResult result);
    
    /**
     * @brief Деструктор блокировки ресурса
     * 
     * @details Автоматически разблокирует ресурс, если он был заблокирован
     */
    ~ResourceLock();
    
    /**
     * @brief Разблокировать ресурс вручную
     * 
     * @return SyncResult Результат операции
     */
    SyncResult unlock();
    
    /**
     * @brief Проверить, успешно ли ресурс был заблокирован
     * 
     * @return true Если ресурс заблокирован
     * @return false Если ресурс не заблокирован
     */
    bool isLocked() const;
    
    /**
     * @brief Получить результат операции блокировки
     * 
     * @return SyncResult Результат операции
     */
    SyncResult getResult() const;
    
private:
    Synchronizer& m_synchronizer;
    std::string m_resourceId;
    LockType m_lockType;
    SyncResult m_result;
    bool m_locked;
};

} // namespace core
} // namespace rtems 