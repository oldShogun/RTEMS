/**
 * @file synchronizer.cpp
 * @brief Реализация синхронизатора для компонентов системы RTEMS
 * 
 * @author RTEMS Team
 * @date 2023
 */

#include "core/synchronizer.hpp"
#include "common/logger.hpp"

namespace rtems {
namespace core {

Synchronizer& Synchronizer::getInstance() {
    static Synchronizer instance;
    return instance;
}

Synchronizer::Synchronizer() {
    RTEMS_LOG_INFO("Synchronizer created");
}

Synchronizer::~Synchronizer() {
    RTEMS_LOG_INFO("Synchronizer destroyed");
}

bool Synchronizer::registerResource(const std::string& resourceId) {
    std::unique_lock<std::mutex> lock(m_resourceMutex);
    
    if (m_resources.find(resourceId) != m_resources.end()) {
        RTEMS_LOG_WARN("Resource {} already registered", resourceId);
        return false;
    }
    
    m_resources[resourceId] = std::make_unique<SyncResource>();
    RTEMS_LOG_INFO("Resource {} registered for synchronization", resourceId);
    return true;
}

bool Synchronizer::unregisterResource(const std::string& resourceId) {
    std::unique_lock<std::mutex> lock(m_resourceMutex);
    
    auto it = m_resources.find(resourceId);
    if (it == m_resources.end()) {
        RTEMS_LOG_WARN("Resource {} not found for unregistration", resourceId);
        return false;
    }
    
    // Проверяем, не заблокирован ли ресурс
    if (it->second->exclusiveLocked || it->second->sharedLocks > 0) {
        RTEMS_LOG_WARN("Cannot unregister resource {} because it is locked", resourceId);
        return false;
    }
    
    m_resources.erase(it);
    RTEMS_LOG_INFO("Resource {} unregistered from synchronization", resourceId);
    return true;
}

SyncResult Synchronizer::lock(const std::string& resourceId, LockType lockType, int timeout) {
    std::unique_lock<std::mutex> resourcesLock(m_resourceMutex);
    
    auto it = m_resources.find(resourceId);
    if (it == m_resources.end()) {
        RTEMS_LOG_WARN("Resource {} not found for locking", resourceId);
        return SyncResult::ERROR;
    }
    
    auto& resource = it->second;
    resourcesLock.unlock();
    
    if (lockType == LockType::EXCLUSIVE) {
        // Проверка, не захвачена ли уже эксклюзивная блокировка
        if (resource->exclusiveLocked) {
            RTEMS_LOG_WARN("Resource {} already has exclusive lock", resourceId);
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
            if (!resource->mutex.try_lock_for(std::chrono::milliseconds(timeout))) {
                return SyncResult::TIMEOUT;
            }
        }
        
        resource->exclusiveLocked = true;
        RTEMS_LOG_DEBUG("Resource {} locked exclusively", resourceId);
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
            if (!resource->mutex.try_lock_shared_for(std::chrono::milliseconds(timeout))) {
                return SyncResult::TIMEOUT;
            }
        }
        
        resource->sharedLocks++;
        RTEMS_LOG_DEBUG("Resource {} locked shared (count: {})", resourceId, resource->sharedLocks.load());
    }
    
    return SyncResult::SUCCESS;
}

SyncResult Synchronizer::unlock(const std::string& resourceId, LockType lockType) {
    std::unique_lock<std::mutex> resourcesLock(m_resourceMutex);
    
    auto it = m_resources.find(resourceId);
    if (it == m_resources.end()) {
        RTEMS_LOG_WARN("Resource {} not found for unlocking", resourceId);
        return SyncResult::ERROR;
    }
    
    auto& resource = it->second;
    resourcesLock.unlock();
    
    if (lockType == LockType::EXCLUSIVE) {
        if (!resource->exclusiveLocked) {
            RTEMS_LOG_WARN("Resource {} does not have exclusive lock to unlock", resourceId);
            return SyncResult::NOT_LOCKED;
        }
        
        resource->exclusiveLocked = false;
        resource->mutex.unlock();
        RTEMS_LOG_DEBUG("Resource {} unlocked exclusively", resourceId);
    } else {
        if (resource->sharedLocks == 0) {
            RTEMS_LOG_WARN("Resource {} does not have shared locks to unlock", resourceId);
            return SyncResult::NOT_LOCKED;
        }
        
        resource->sharedLocks--;
        resource->mutex.unlock_shared();
        RTEMS_LOG_DEBUG("Resource {} unlocked shared (count: {})", resourceId, resource->sharedLocks.load());
    }
    
    return SyncResult::SUCCESS;
}

bool Synchronizer::isLocked(const std::string& resourceId) const {
    std::unique_lock<std::mutex> lock(m_resourceMutex);
    
    auto it = m_resources.find(resourceId);
    if (it == m_resources.end()) {
        RTEMS_LOG_WARN("Resource {} not found for lock check", resourceId);
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
        RTEMS_LOG_WARN("Resource {} not found for waiting on condition", resourceId);
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
        RTEMS_LOG_WARN("Resource {} not found for notification", resourceId);
        return SyncResult::ERROR;
    }
    
    auto& resource = it->second;
    resourcesLock.unlock();
    
    resource->condition.notify_one();
    RTEMS_LOG_DEBUG("Resource {} notified one waiting thread", resourceId);
    return SyncResult::SUCCESS;
}

SyncResult Synchronizer::notifyAll(const std::string& resourceId) {
    std::unique_lock<std::mutex> resourcesLock(m_resourceMutex);
    
    auto it = m_resources.find(resourceId);
    if (it == m_resources.end()) {
        RTEMS_LOG_WARN("Resource {} not found for notification", resourceId);
        return SyncResult::ERROR;
    }
    
    auto& resource = it->second;
    resourcesLock.unlock();
    
    resource->condition.notify_all();
    RTEMS_LOG_DEBUG("Resource {} notified all waiting threads", resourceId);
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