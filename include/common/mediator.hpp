/**
 * @file mediator.hpp
 * @brief Медиатор компонентов системы RTEMS
 * @details Обеспечивает взаимодействие между компонентами системы
 * 
 * @author RTEMS Team
 * @date 2023
 */

#pragma once

#include <memory>
#include <string>
#include <functional>
#include <map>
#include <mutex>
#include <vector>
#include <any>
#include <optional>
#include <nlohmann/json.hpp>

#include "common/icomponent.hpp"
#include "common/logger.hpp"

namespace rtems {
namespace common {

/**
 * @brief Тип идентификатора сообщения
 */
using MessageId = std::string;

/**
 * @brief Структура сообщения для обмена данными между компонентами
 */
struct Message {
    MessageId id;              ///< Идентификатор сообщения
    ComponentId sender;        ///< Идентификатор отправителя
    ComponentId receiver;      ///< Идентификатор получателя (пустая строка для широковещательных сообщений)
    std::any data;             ///< Данные сообщения (могут быть любого типа)
    nlohmann::json metadata;   ///< Метаданные сообщения
    
    /**
     * @brief Создать новое сообщение
     * 
     * @param id Идентификатор сообщения
     * @param sender Идентификатор отправителя
     * @param receiver Идентификатор получателя (пустая строка для широковещательных сообщений)
     * @param data Данные сообщения
     * @param metadata Метаданные сообщения
     */
    template<typename T>
    Message(const MessageId& id, const ComponentId& sender, const ComponentId& receiver, 
            const T& data, const nlohmann::json& metadata = nlohmann::json())
        : id(id), sender(sender), receiver(receiver), data(data), metadata(metadata) {}
    
    /**
     * @brief Получить данные сообщения с приведением к указанному типу
     * 
     * @tparam T Тип данных
     * @return std::optional<T> Данные указанного типа или std::nullopt, если приведение невозможно
     */
    template<typename T>
    std::optional<T> getDataAs() const {
        try {
            return std::any_cast<T>(data);
        } catch (const std::bad_any_cast&) {
            return std::nullopt;
        }
    }
};

/**
 * @brief Тип обработчика сообщений
 */
using MessageHandler = std::function<void(const Message&)>;

/**
 * @brief Медиатор для взаимодействия компонентов системы RTEMS
 * 
 * @details Медиатор реализует паттерн "Посредник" и обеспечивает обмен сообщениями между компонентами.
 *          Компоненты могут отправлять сообщения другим компонентам или подписываться на определенные
 *          типы сообщений через медиатор, не имея прямых зависимостей друг от друга.
 */
class Mediator {
public:
    /**
     * @brief Получить экземпляр медиатора (Singleton)
     * 
     * @return Mediator& Ссылка на экземпляр медиатора
     */
    static Mediator& instance();
    
    /**
     * @brief Инициализировать медиатор
     * 
     * @param config Конфигурация медиатора
     * @return true если инициализация прошла успешно
     * @return false если возникли ошибки при инициализации
     */
    bool initialize(const nlohmann::json& config = nlohmann::json());
    
    /**
     * @brief Деинициализировать медиатор и освободить ресурсы
     */
    void shutdown();
    
    /**
     * @brief Зарегистрировать обработчик сообщений для компонента
     * 
     * @param componentId Идентификатор компонента
     * @param messageId Идентификатор сообщения
     * @param handler Обработчик сообщений
     * @return true если регистрация прошла успешно
     * @return false если возникли ошибки при регистрации
     */
    bool registerHandler(const ComponentId& componentId, const MessageId& messageId, MessageHandler handler);
    
    /**
     * @brief Удалить обработчик сообщений для компонента
     * 
     * @param componentId Идентификатор компонента
     * @param messageId Идентификатор сообщения
     * @return true если удаление прошло успешно
     * @return false если обработчик не найден
     */
    bool unregisterHandler(const ComponentId& componentId, const MessageId& messageId);
    
    /**
     * @brief Удалить все обработчики сообщений для компонента
     * 
     * @param componentId Идентификатор компонента
     * @return size_t Количество удаленных обработчиков
     */
    size_t unregisterAllHandlers(const ComponentId& componentId);
    
    /**
     * @brief Отправить сообщение
     * 
     * @param message Сообщение для отправки
     * @return true если сообщение успешно отправлено
     * @return false если возникли ошибки при отправке
     */
    bool sendMessage(const Message& message);
    
    /**
     * @brief Отправить сообщение с созданием объекта Message
     * 
     * @tparam T Тип данных сообщения
     * @param messageId Идентификатор сообщения
     * @param sender Идентификатор отправителя
     * @param receiver Идентификатор получателя (пустая строка для широковещательных сообщений)
     * @param data Данные сообщения
     * @param metadata Метаданные сообщения
     * @return true если сообщение успешно отправлено
     * @return false если возникли ошибки при отправке
     */
    template<typename T>
    bool sendMessage(const MessageId& messageId, const ComponentId& sender, 
                    const ComponentId& receiver, const T& data, 
                    const nlohmann::json& metadata = nlohmann::json()) {
        return sendMessage(Message(messageId, sender, receiver, data, metadata));
    }
    
    /**
     * @brief Отправить широковещательное сообщение всем компонентам
     * 
     * @tparam T Тип данных сообщения
     * @param messageId Идентификатор сообщения
     * @param sender Идентификатор отправителя
     * @param data Данные сообщения
     * @param metadata Метаданные сообщения
     * @return true если сообщение успешно отправлено
     * @return false если возникли ошибки при отправке
     */
    template<typename T>
    bool broadcastMessage(const MessageId& messageId, const ComponentId& sender, 
                         const T& data, const nlohmann::json& metadata = nlohmann::json()) {
        return sendMessage(Message(messageId, sender, "", data, metadata));
    }
    
    /**
     * @brief Проверить, инициализирован ли медиатор
     * 
     * @return true если медиатор инициализирован
     * @return false если медиатор не инициализирован
     */
    bool isInitialized() const;
    
private:
    /**
     * @brief Конструктор по умолчанию (приватный для Singleton)
     */
    Mediator();
    
    /**
     * @brief Копирующий конструктор (запрещен для Singleton)
     */
    Mediator(const Mediator&) = delete;
    
    /**
     * @brief Оператор присваивания (запрещен для Singleton)
     */
    Mediator& operator=(const Mediator&) = delete;
    
    /**
     * @brief Структура для хранения информации о компоненте и его обработчике
     */
    struct HandlerInfo {
        ComponentId componentId;    ///< Идентификатор компонента
        MessageHandler handler;     ///< Обработчик сообщений
    };
    
    std::map<MessageId, std::vector<HandlerInfo>> m_handlers;   ///< Карта обработчиков по идентификатору сообщения
    bool m_initialized;                                         ///< Флаг инициализации
    mutable std::mutex m_mutex;                                 ///< Мьютекс для потокобезопасности
};

} // namespace common
} // namespace rtems 