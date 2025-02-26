/**
 * @file mediator.cpp
 * @brief Реализация медиатора компонентов системы RTEMS
 * 
 * @author RTEMS Team
 * @date 2023
 */

#include "common/mediator.hpp"
#include "common/logger.hpp"

namespace rtems {
namespace common {

// Статический метод для доступа к экземпляру синглтона
Mediator& Mediator::instance() {
    static Mediator instance;
    return instance;
}

// Конструктор
Mediator::Mediator() : m_initialized(false) {
    LOG_INFO("Mediator создан");
}

// Инициализация медиатора
bool Mediator::initialize(const nlohmann::json& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_initialized) {
        LOG_WARNING("Mediator уже инициализирован");
        return true;
    }
    
    try {
        LOG_INFO("Инициализация Mediator");
        
        // Дополнительные действия по инициализации можно добавить здесь
        // Например, предварительная регистрация известных типов сообщений
        
        m_initialized = true;
        LOG_INFO("Mediator успешно инициализирован");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Ошибка при инициализации Mediator: {}", e.what());
        return false;
    }
}

// Деинициализация и освобождение ресурсов
void Mediator::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        LOG_WARNING("Mediator не был инициализирован");
        return;
    }
    
    LOG_INFO("Очистка обработчиков сообщений");
    m_handlers.clear();
    
    m_initialized = false;
    LOG_INFO("Mediator успешно деинициализирован");
}

// Регистрация обработчика сообщений
bool Mediator::registerHandler(const ComponentId& componentId, const MessageId& messageId, MessageHandler handler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("Попытка зарегистрировать обработчик для неинициализированного Mediator");
        return false;
    }
    
    if (!handler) {
        LOG_ERROR("Попытка зарегистрировать nullptr обработчик для компонента: {}, сообщение: {}", componentId, messageId);
        return false;
    }
    
    // Проверка на дублирование обработчика
    auto& handlers = m_handlers[messageId];
    for (const auto& info : handlers) {
        if (info.componentId == componentId) {
            LOG_WARNING("Обработчик для компонента: {}, сообщение: {} уже зарегистрирован. Обновляем.", componentId, messageId);
            
            // Обновляем существующий обработчик
            const_cast<HandlerInfo&>(info).handler = handler;
            return true;
        }
    }
    
    // Добавляем новый обработчик
    HandlerInfo info;
    info.componentId = componentId;
    info.handler = handler;
    handlers.push_back(info);
    
    LOG_INFO("Зарегистрирован обработчик для компонента: {}, сообщение: {}", componentId, messageId);
    return true;
}

// Удаление обработчика сообщений
bool Mediator::unregisterHandler(const ComponentId& componentId, const MessageId& messageId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("Попытка удалить обработчик для неинициализированного Mediator");
        return false;
    }
    
    auto it = m_handlers.find(messageId);
    if (it == m_handlers.end()) {
        LOG_WARNING("Попытка удалить несуществующий обработчик для сообщения: {}", messageId);
        return false;
    }
    
    auto& handlers = it->second;
    for (auto handlerIt = handlers.begin(); handlerIt != handlers.end(); ++handlerIt) {
        if (handlerIt->componentId == componentId) {
            handlers.erase(handlerIt);
            LOG_INFO("Удален обработчик для компонента: {}, сообщение: {}", componentId, messageId);
            
            // Если больше нет обработчиков для этого сообщения, удаляем запись
            if (handlers.empty()) {
                m_handlers.erase(it);
            }
            
            return true;
        }
    }
    
    LOG_WARNING("Обработчик для компонента: {}, сообщение: {} не найден", componentId, messageId);
    return false;
}

// Удаление всех обработчиков для компонента
size_t Mediator::unregisterAllHandlers(const ComponentId& componentId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("Попытка удалить обработчики для неинициализированного Mediator");
        return 0;
    }
    
    size_t removedCount = 0;
    
    // Проходим по всем типам сообщений
    for (auto it = m_handlers.begin(); it != m_handlers.end();) {
        auto& handlers = it->second;
        
        // Удаляем обработчики для указанного компонента
        for (auto handlerIt = handlers.begin(); handlerIt != handlers.end();) {
            if (handlerIt->componentId == componentId) {
                handlerIt = handlers.erase(handlerIt);
                removedCount++;
            } else {
                ++handlerIt;
            }
        }
        
        // Если больше нет обработчиков для этого сообщения, удаляем запись
        if (handlers.empty()) {
            it = m_handlers.erase(it);
        } else {
            ++it;
        }
    }
    
    LOG_INFO("Удалено {} обработчиков для компонента: {}", removedCount, componentId);
    return removedCount;
}

// Отправка сообщения
bool Mediator::sendMessage(const Message& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("Попытка отправить сообщение через неинициализированный Mediator");
        return false;
    }
    
    bool delivered = false;
    
    // Проверяем, есть ли обработчики для данного типа сообщения
    auto it = m_handlers.find(message.id);
    if (it == m_handlers.end()) {
        LOG_DEBUG("Нет обработчиков для сообщения: {}", message.id);
        return false;
    }
    
    // Если указан получатель, доставляем сообщение только ему
    if (!message.receiver.empty()) {
        bool foundReceiver = false;
        
        for (const auto& info : it->second) {
            if (info.componentId == message.receiver) {
                foundReceiver = true;
                
                try {
                    // Вызываем обработчик в блоке try-catch для обработки исключений
                    info.handler(message);
                    delivered = true;
                    LOG_DEBUG("Сообщение: {} доставлено от: {} получателю: {}", 
                        message.id, message.sender, message.receiver);
                } catch (const std::exception& e) {
                    LOG_ERROR("Исключение при обработке сообщения: {} компонентом: {}: {}", 
                        message.id, info.componentId, e.what());
                }
                
                break;  // Завершаем поиск после доставки
            }
        }
        
        if (!foundReceiver) {
            LOG_WARNING("Получатель: {} не зарегистрирован для приема сообщения: {}", 
                message.receiver, message.id);
        }
    }
    // Иначе доставляем всем подписчикам (широковещательное сообщение)
    else {
        for (const auto& info : it->second) {
            // Не отправляем сообщение обратно отправителю
            if (info.componentId == message.sender) {
                continue;
            }
            
            try {
                // Вызываем обработчик в блоке try-catch для обработки исключений
                info.handler(message);
                delivered = true;
                LOG_DEBUG("Широковещательное сообщение: {} от: {} доставлено получателю: {}", 
                    message.id, message.sender, info.componentId);
            } catch (const std::exception& e) {
                LOG_ERROR("Исключение при обработке широковещательного сообщения: {} компонентом: {}: {}", 
                    message.id, info.componentId, e.what());
            }
        }
    }
    
    return delivered;
}

// Проверка инициализации
bool Mediator::isInitialized() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_initialized;
}

} // namespace common
} // namespace rtems 