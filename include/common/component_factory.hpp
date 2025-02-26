/**
 * @file component_factory.hpp
 * @brief Фабрика компонентов системы RTEMS
 * @details Предоставляет функционал для создания различных компонентов системы
 * 
 * @author RTEMS Team
 * @date 2023
 */

#pragma once

#include <map>
#include <functional>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

#include "common/icomponent.hpp"
#include "common/logger.hpp"

namespace rtems {
namespace common {

/**
 * @brief Фабрика для создания компонентов системы RTEMS
 * 
 * @details Класс реализует паттерн "Фабричный метод" для создания различных 
 *          компонентов системы по их идентификаторам типа. 
 *          Фабрика позволяет регистрировать и создавать компоненты различных типов.
 */
class ComponentFactory {
public:
    /**
     * @brief Тип функции-создателя компонента
     */
    using CreatorFunction = std::function<ComponentPtr()>;
    
    /**
     * @brief Получить экземпляр фабрики (Singleton)
     * 
     * @return ComponentFactory& Ссылка на экземпляр фабрики
     */
    static ComponentFactory& instance();
    
    /**
     * @brief Зарегистрировать создателя компонента
     * 
     * @param componentType Тип компонента
     * @param typeName Строковое имя типа компонента
     * @param creator Функция-создатель компонента
     * @return true если регистрация прошла успешно
     * @return false если компонент с таким типом уже зарегистрирован
     */
    bool registerCreator(ComponentType componentType, const std::string& typeName, CreatorFunction creator);
    
    /**
     * @brief Создать компонент указанного типа
     * 
     * @param componentType Тип компонента для создания
     * @param config Конфигурация компонента
     * @return ComponentPtr Указатель на созданный компонент или nullptr, если создание не удалось
     */
    ComponentPtr createComponent(ComponentType componentType, const nlohmann::json& config = nlohmann::json());
    
    /**
     * @brief Создать компонент указанного типа по строковому имени типа
     * 
     * @param typeName Строковое имя типа компонента
     * @param config Конфигурация компонента
     * @return ComponentPtr Указатель на созданный компонент или nullptr, если создание не удалось
     */
    ComponentPtr createComponentByName(const std::string& typeName, const nlohmann::json& config = nlohmann::json());
    
    /**
     * @brief Получить строковое имя типа компонента
     * 
     * @param componentType Тип компонента
     * @return std::string Строковое имя типа или "UNKNOWN", если тип не зарегистрирован
     */
    std::string getComponentTypeName(ComponentType componentType) const;
    
    /**
     * @brief Получить тип компонента по строковому имени
     * 
     * @param typeName Строковое имя типа компонента
     * @return ComponentType Тип компонента или ComponentType::UNDEFINED, если тип не найден
     */
    ComponentType getComponentTypeByName(const std::string& typeName) const;
    
private:
    /**
     * @brief Конструктор по умолчанию (приватный для Singleton)
     */
    ComponentFactory();
    
    /**
     * @brief Копирующий конструктор (запрещен для Singleton)
     */
    ComponentFactory(const ComponentFactory&) = delete;
    
    /**
     * @brief Оператор присваивания (запрещен для Singleton)
     */
    ComponentFactory& operator=(const ComponentFactory&) = delete;
    
    /**
     * @brief Структура для хранения информации о типе компонента
     */
    struct ComponentTypeInfo {
        std::string typeName;       ///< Строковое имя типа
        CreatorFunction creator;    ///< Функция-создатель компонента
    };
    
    std::map<ComponentType, ComponentTypeInfo> m_creators;    ///< Карта создателей компонентов по типу
    std::map<std::string, ComponentType> m_typeNameMap;       ///< Карта соответствия строковых имен типам
};

} // namespace common
} // namespace rtems 