/**
 * @file icomponent.hpp
 * @brief Интерфейс базового компонента системы RTEMS
 * @details Определяет базовый интерфейс, который должны реализовывать все компоненты системы
 * 
 * @author RTEMS Team
 * @date 2023
 */

#pragma once

#include <string>
#include <memory>
#include <nlohmann/json.hpp>

namespace rtems {
namespace common {

/**
 * @brief Уникальный идентификатор компонента
 */
using ComponentId = std::string;

/**
 * @brief Перечисление типов компонентов системы
 */
enum class ComponentType {
    UNDEFINED,           ///< Неопределенный тип
    DATA_SOURCE,         ///< Источник данных
    STORAGE,             ///< Хранилище данных
    MONITOR,             ///< Компонент мониторинга
    PREDICTOR,           ///< Компонент прогнозирования
    ACTIVATOR,           ///< Компонент активации
    ANALYZER,            ///< Компонент анализа
    SIMULATOR,           ///< Компонент моделирования
    CONTROLLER,          ///< Компонент управления
    OUTPUT_HANDLER       ///< Компонент вывода
};

/**
 * @brief Базовый интерфейс для всех компонентов системы RTEMS
 * 
 * @details Интерфейс, определяющий общие методы для всех компонентов системы:
 *          - получение идентификатора компонента
 *          - получение типа компонента
 *          - инициализация компонента
 *          - запуск и остановка компонента
 *          - проверка статуса компонента
 */
class IComponent {
public:
    /**
     * @brief Виртуальный деструктор
     */
    virtual ~IComponent() = default;

    /**
     * @brief Получить идентификатор компонента
     * 
     * @return ComponentId Уникальный идентификатор компонента
     */
    virtual ComponentId getId() const = 0;

    /**
     * @brief Получить тип компонента
     * 
     * @return ComponentType Тип компонента
     */
    virtual ComponentType getType() const = 0;

    /**
     * @brief Инициализировать компонент
     * 
     * @param config Конфигурация компонента в формате JSON
     * @return true если инициализация прошла успешно
     * @return false если возникли ошибки при инициализации
     * @throws InitializationException при критических ошибках инициализации
     */
    virtual bool initialize(const nlohmann::json& config) = 0;

    /**
     * @brief Запустить компонент
     * 
     * @return true если запуск прошел успешно
     * @return false если возникли ошибки при запуске
     */
    virtual bool start() = 0;

    /**
     * @brief Остановить компонент
     * 
     * @return true если остановка прошла успешно
     * @return false если возникли ошибки при остановке
     */
    virtual bool stop() = 0;

    /**
     * @brief Проверить, запущен ли компонент
     * 
     * @return true если компонент запущен
     * @return false если компонент остановлен
     */
    virtual bool isRunning() const = 0;

    /**
     * @brief Проверить, инициализирован ли компонент
     * 
     * @return true если компонент инициализирован
     * @return false если компонент не инициализирован
     */
    virtual bool isInitialized() const = 0;
};

/**
 * @typedef ComponentPtr
 * @brief Умный указатель на компонент
 */
using ComponentPtr = std::shared_ptr<IComponent>;

} // namespace common
} // namespace rtems 