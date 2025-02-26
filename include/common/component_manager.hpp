/**
 * @file component_manager.hpp
 * @brief Менеджер компонентов системы RTEMS
 * @details Предоставляет функционал для управления жизненным циклом компонентов системы
 * 
 * @author RTEMS Team
 * @date 2023
 */

#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <nlohmann/json.hpp>

#include "common/icomponent.hpp"
#include "common/component_factory.hpp"
#include "common/logger.hpp"

namespace rtems {
namespace common {

/**
 * @brief Менеджер компонентов системы RTEMS
 * 
 * @details Класс осуществляет управление жизненным циклом всех компонентов системы:
 *          - создание и инициализация компонентов
 *          - запуск и остановка компонентов
 *          - доступ к компонентам по идентификатору
 *          - удаление компонентов
 */
class ComponentManager {
public:
    /**
     * @brief Получить экземпляр менеджера компонентов (Singleton)
     * 
     * @return ComponentManager& Ссылка на экземпляр менеджера компонентов
     */
    static ComponentManager& instance();
    
    /**
     * @brief Инициализировать менеджер компонентов
     * 
     * @param config Конфигурация менеджера компонентов
     * @return true если инициализация прошла успешно
     * @return false если возникли ошибки при инициализации
     */
    bool initialize(const nlohmann::json& config);
    
    /**
     * @brief Деинициализировать менеджер компонентов и освободить ресурсы
     */
    void shutdown();
    
    /**
     * @brief Добавить компонент в менеджер
     * 
     * @param component Указатель на компонент
     * @return true если компонент успешно добавлен
     * @return false если компонент с таким ID уже существует или указатель nullptr
     */
    bool addComponent(ComponentPtr component);
    
    /**
     * @brief Создать и добавить компонент указанного типа
     * 
     * @param componentType Тип компонента
     * @param config Конфигурация компонента
     * @return ComponentPtr Указатель на созданный компонент или nullptr, если создание не удалось
     */
    ComponentPtr createAndAddComponent(ComponentType componentType, const nlohmann::json& config = nlohmann::json());
    
    /**
     * @brief Создать и добавить компонент указанного типа по строковому имени
     * 
     * @param typeName Строковое имя типа компонента
     * @param config Конфигурация компонента
     * @return ComponentPtr Указатель на созданный компонент или nullptr, если создание не удалось
     */
    ComponentPtr createAndAddComponentByName(const std::string& typeName, const nlohmann::json& config = nlohmann::json());
    
    /**
     * @brief Получить компонент по идентификатору
     * 
     * @param componentId Идентификатор компонента
     * @return ComponentPtr Указатель на компонент или nullptr, если компонент не найден
     */
    ComponentPtr getComponent(const ComponentId& componentId);
    
    /**
     * @brief Получить компонент по идентификатору с приведением к указанному типу
     * 
     * @tparam T Тип компонента, к которому нужно привести
     * @param componentId Идентификатор компонента
     * @return std::shared_ptr<T> Указатель на компонент приведенного типа или nullptr
     */
    template<typename T>
    std::shared_ptr<T> getComponentAs(const ComponentId& componentId) {
        ComponentPtr component = getComponent(componentId);
        if (!component) {
            return nullptr;
        }
        return std::dynamic_pointer_cast<T>(component);
    }
    
    /**
     * @brief Получить все компоненты указанного типа
     * 
     * @param componentType Тип компонентов для получения
     * @return std::vector<ComponentPtr> Вектор указателей на компоненты указанного типа
     */
    std::vector<ComponentPtr> getComponentsByType(ComponentType componentType);
    
    /**
     * @brief Удалить компонент по идентификатору
     * 
     * @param componentId Идентификатор компонента
     * @return true если компонент успешно удален
     * @return false если компонент не найден
     */
    bool removeComponent(const ComponentId& componentId);
    
    /**
     * @brief Запустить все компоненты
     * 
     * @return true если все компоненты успешно запущены
     * @return false если возникли ошибки при запуске
     */
    bool startAllComponents();
    
    /**
     * @brief Остановить все компоненты
     * 
     * @return true если все компоненты успешно остановлены
     * @return false если возникли ошибки при остановке
     */
    bool stopAllComponents();
    
    /**
     * @brief Получить количество зарегистрированных компонентов
     * 
     * @return size_t Количество компонентов
     */
    size_t getComponentCount() const;
    
    /**
     * @brief Проверить, инициализирован ли менеджер компонентов
     * 
     * @return true если менеджер компонентов инициализирован
     * @return false если менеджер компонентов не инициализирован
     */
    bool isInitialized() const;
    
private:
    /**
     * @brief Конструктор по умолчанию (приватный для Singleton)
     */
    ComponentManager();
    
    /**
     * @brief Копирующий конструктор (запрещен для Singleton)
     */
    ComponentManager(const ComponentManager&) = delete;
    
    /**
     * @brief Оператор присваивания (запрещен для Singleton)
     */
    ComponentManager& operator=(const ComponentManager&) = delete;
    
    std::unordered_map<ComponentId, ComponentPtr> m_components;  ///< Карта компонентов по ID
    bool m_initialized;                                          ///< Флаг инициализации
    mutable std::mutex m_mutex;                                  ///< Мьютекс для потокобезопасности
};

} // namespace common
} // namespace rtems 