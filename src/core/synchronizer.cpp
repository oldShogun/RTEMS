/**
 * @file synchronizer.cpp
 * @brief Реализация синхронизатора для компонентов системы RTEMS
 * 
 * @author RTEMS Team
 * @date 2023
 */

#include "core/synchronizer.hpp"
#include "common/logger.hpp"
#include <sstream>

namespace rtems {
namespace core {

Synchronizer& Synchronizer::getInstance() {
    static Synchronizer instance;
    return instance;
}

Synchronizer::Synchronizer() {
    LOG_INFO("Synchronizer created");
}

Synchronizer::~Synchronizer() {
    LOG_INFO("Synchronizer destroyed");
}

bool Synchronizer::registerResource(const std::string& resourceId) {
    std::unique_lock<std::mutex> lock(m_resourceMutex);
    
    if (m_resources.find(resourceId) != m_resources.end()) {
        std::ostringstream oss;
        oss << "Resource " << resourceId << " already registered";
        LOG_WARNING(oss.str());
        return false;
    }
    
    m_resources[resourceId] = std::make_unique<SyncResource>();
    std::ostringstream oss;
    oss << "Resource " << resourceId << " registered for synchronization";
    LOG_INFO(oss.str());
    return true;
}

bool Synchronizer::unregisterResource(const std::string& resourceId) {
    std::unique_lock<std::mutex> lock(m_resourceMutex);
    
    auto it = m_resources.find(resourceId);
    if (it == m_resources.end()) {
        std::ostringstream oss;
        oss << "Resource " << resourceId << " not found for unregistration";
        LOG_WARNING(oss.str());
        return false;
    }
    
    // Проверяем, не заблокирован ли ресурс
    if (it->second->exclusiveLocked || it->second->sharedLocks > 0) {
        std::ostringstream oss;
        oss << "Cannot unregister resource " << resourceId << " because it is locked";
        LOG_WARNING(oss.str());
        return false;
    }
    
    m_resources.erase(it);
    std::ostringstream oss;
    oss << "Resource " << resourceId << " unregistered from synchronization";
    LOG_INFO(oss.str());
    return true;
}

SyncResult Synchronizer::lock(const std::string& resourceId, LockType lockType, int timeout) {
    std::unique_lock<std::mutex> resourcesLock(m_resourceMutex);
    
    auto it = m_resources.find(resourceId);
    if (it == m_resources.end()) {
        std::ostringstream oss;
        oss << "Resource " << resourceId << " not found for locking";
        LOG_WARNING(oss.str());
        return SyncResult::ERROR;
    }
    
    auto& resource = it->second;
    resourcesLock.unlock();
    
    if (lockType == LockType::EXCLUSIVE) {
        // Проверка, не захвачена ли уже эксклюзивная блокировка
        if (resource->exclusiveLocked) {
            std::ostringstream oss;
            oss << "Resource " << resourceId << " already has exclusive lock";
            LOG_WARNING(oss.str());
            return SyncResult::ALREADY_LOCKED;
        }
        
        if (timeout < 0) {
            // Бесконечное ожидание
            resource->mutex.lock();
        } else if (timeout == 0) {
            // Без ожидания
            if (!resource->mutex.try_lock()) {
                return SyncResult::TIMEOUT;
            }
        } else {
            // Ожидание с таймаутом
            if (!resource->mutex.try_lock()) {
                // Если нет поддержки try_lock_for, делаем упрощенную реализацию
                // с неблокирующими попытками захвата
                auto start = std::chrono::steady_clock::now();
                bool locked = false;
                
                while (!locked) {
                    locked = resource->mutex.try_lock();
                    if (locked) break;
                    
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
                    if (elapsed.count() >= timeout) {
                        return SyncResult::TIMEOUT;
                    }
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Небольшая задержка
                }
            }
        }
        
        resource->exclusiveLocked = true;
        std::ostringstream oss;
        oss << "Resource " << resourceId << " locked exclusively";
        LOG_DEBUG(oss.str());
    } else {
        // Разделяемая блокировка
        if (timeout < 0) {
            // Бесконечное ожидание
            resource->mutex.lock_shared();
        } else if (timeout == 0) {
            // Без ожидания
            if (!resource->mutex.try_lock_shared()) {
                return SyncResult::TIMEOUT;
            }
        } else {
            // Ожидание с таймаутом
            if (!resource->mutex.try_lock_shared()) {
                // Если нет поддержки try_lock_shared_for, делаем упрощенную реализацию
                // с неблокирующими попытками захвата
                auto start = std::chrono::steady_clock::now();
                bool locked = false;
                
                while (!locked) {
                    locked = resource->mutex.try_lock_shared();
                    if (locked) break;
                    
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
                    if (elapsed.count() >= timeout) {
                        return SyncResult::TIMEOUT;
                    }
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Небольшая задержка
                }
            }
        }
        
        resource->sharedLocks++;
        std::ostringstream oss;
        oss << "Resource " << resourceId << " locked shared (count: " << resource->sharedLocks.load() << ")";
        LOG_DEBUG(oss.str());
    }
    
    return SyncResult::SUCCESS;
}

SyncResult Synchronizer::unlock(const std::string& resourceId, LockType lockType) {
    std::unique_lock<std::mutex> resourcesLock(m_resourceMutex);
    
    auto it = m_resources.find(resourceId);
    if (it == m_resources.end()) {
        std::ostringstream oss;
        oss << "Resource " << resourceId << " not found for unlocking";
        LOG_WARNING(oss.str());
        return SyncResult::ERROR;
    }
    
    auto& resource = it->second;
    resourcesLock.unlock();
    
    if (lockType == LockType::EXCLUSIVE) {
        if (!resource->exclusiveLocked) {
            std::ostringstream oss;
            oss << "Resource " << resourceId << " does not have exclusive lock to unlock";
            LOG_WARNING(oss.str());
            return SyncResult::NOT_LOCKED;
        }
        
        resource->exclusiveLocked = false;
        resource->mutex.unlock();
        std::ostringstream oss;
        oss << "Resource " << resourceId << " unlocked exclusively";
        LOG_DEBUG(oss.str());
    } else {
        if (resource->sharedLocks == 0) {
            std::ostringstream oss;
            oss << "Resource " << resourceId << " does not have shared locks to unlock";
            LOG_WARNING(oss.str());
            return SyncResult::NOT_LOCKED;
        }
        
        resource->sharedLocks--;
        resource->mutex.unlock_shared();
        std::ostringstream oss;
        oss << "Resource " << resourceId << " unlocked shared (count: " << resource->sharedLocks.load() << ")";
        LOG_DEBUG(oss.str());
    }
    
    return SyncResult::SUCCESS;
}

bool Synchronizer::isLocked(const std::string& resourceId) const {
    std::unique_lock<std::mutex> lock(m_resourceMutex);
    
    auto it = m_resources.find(resourceId);
    if (it == m_resources.end()) {
        std::ostringstream oss;
        oss << "Resource " << resourceId << " not found for lock check";
        LOG_WARNING(oss.str());
        return false;
    }
    
    return it->second->exclusiveLocked || it->second->sharedLocks > 0;
}

std::unique_ptr<ResourceLock> Synchronizer::createLock(
    const std::string& resourceId, 
    LockType lockType, 
    int timeout) {
    
    SyncResult result = lock(resourceId, lockType, timeout);
    return std::make_unique<ResourceLock>(*this, resourceId, lockType, result);
}

SyncResult Synchronizer::waitForCondition(
    const std::string& resourceId, 
    std::function<bool()> predicate, 
    int timeout) {
    
    std::unique_lock<std::mutex> resourcesLock(m_resourceMutex);
    
    auto it = m_resources.find(resourceId);
    if (it == m_resources.end()) {
        std::ostringstream oss;
        oss << "Resource " << resourceId << " not found for waiting on condition";
        LOG_WARNING(oss.str());
        return SyncResult::ERROR;
    }
    
    auto& resource = it->second;
    resourcesLock.unlock();
    
    // Сначала пытаемся получить эксклюзивную блокировку
    std::unique_lock<std::shared_mutex> lock(resource->mutex);
    
    if (timeout < 0) {
        // Бесконечное ожидание
        resource->condition.wait(lock, predicate);
        return SyncResult::SUCCESS;
    } else {
        // Ожидание с таймаутом
        if (resource->condition.wait_for(lock, std::chrono::milliseconds(timeout), predicate)) {
            return SyncResult::SUCCESS;
        } else {
            return SyncResult::TIMEOUT;
        }
    }
}

SyncResult Synchronizer::notifyOne(const std::string& resourceId) {
    std::unique_lock<std::mutex> resourcesLock(m_resourceMutex);
    
    auto it = m_resources.find(resourceId);
    if (it == m_resources.end()) {
        std::ostringstream oss;
        oss << "Resource " << resourceId << " not found for notification";
        LOG_WARNING(oss.str());
        return SyncResult::ERROR;
    }
    
    auto& resource = it->second;
    resourcesLock.unlock();
    
    resource->condition.notify_one();
    std::ostringstream oss;
    oss << "Resource " << resourceId << " notified one waiting thread";
    LOG_DEBUG(oss.str());
    return SyncResult::SUCCESS;
}

SyncResult Synchronizer::notifyAll(const std::string& resourceId) {
    std::unique_lock<std::mutex> resourcesLock(m_resourceMutex);
    
    auto it = m_resources.find(resourceId);
    if (it == m_resources.end()) {
        std::ostringstream oss;
        oss << "Resource " << resourceId << " not found for notification";
        LOG_WARNING(oss.str());
        return SyncResult::ERROR;
    }
    
    auto& resource = it->second;
    resourcesLock.unlock();
    
    resource->condition.notify_all();
    std::ostringstream oss;
    oss << "Resource " << resourceId << " notified all waiting threads";
    LOG_DEBUG(oss.str());
    return SyncResult::SUCCESS;
}

size_t Synchronizer::getResourceCount() const {
    std::unique_lock<std::mutex> lock(m_resourceMutex);
    return m_resources.size();
}

// Реализация класса ResourceLock

ResourceLock::ResourceLock(
    Synchronizer& synchronizer, 
    const std::string& resourceId, 
    LockType lockType,
    SyncResult result)
    : m_synchronizer(synchronizer)
    , m_resourceId(resourceId)
    , m_lockType(lockType)
    , m_result(result)
    , m_locked(result == SyncResult::SUCCESS)
{
}

ResourceLock::~ResourceLock() {
    if (m_locked) {
        unlock();
    }
}

SyncResult ResourceLock::unlock() {
    if (!m_locked) {
        return SyncResult::NOT_LOCKED;
    }
    
    SyncResult result = m_synchronizer.unlock(m_resourceId, m_lockType);
    if (result == SyncResult::SUCCESS) {
        m_locked = false;
    }
    
    return result;
}

bool ResourceLock::isLocked() const {
    return m_locked;
}

SyncResult ResourceLock::getResult() const {
    return m_result;
}

} // namespace core
} // namespace rtems 