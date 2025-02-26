#pragma once

#include <stdexcept>
#include <string>

/**
 * @namespace rtems
 * @brief Корневое пространство имён для системы моделирования электромеханических систем в реальном времени
 */
namespace rtems {
/**
 * @namespace rtems::common
 * @brief Общие компоненты, используемые всеми модулями системы
 */
namespace common {

/**
 * @class RTEMSException
 * @brief Базовый класс исключений для системы RTEMS
 *
 * Служит основой для всех специфичных исключений в системе.
 */
class RTEMSException : public std::runtime_error {
public:
    /**
     * @brief Конструктор с сообщением об ошибке
     * @param message Текстовое сообщение, описывающее ошибку
     */
    explicit RTEMSException(const std::string& message) 
        : std::runtime_error(message) {}
};

/**
 * @class InitializationException
 * @brief Исключение, возникающее при ошибках инициализации
 *
 * Генерируется, когда происходит ошибка при инициализации компонентов системы.
 */
class InitializationException : public RTEMSException {
public:
    /**
     * @brief Конструктор с сообщением об ошибке
     * @param message Текстовое сообщение, описывающее ошибку инициализации
     */
    explicit InitializationException(const std::string& message) 
        : RTEMSException("Initialization error: " + message) {}
};

/**
 * @class ConfigurationException
 * @brief Исключение, возникающее при ошибках конфигурации
 *
 * Генерируется при проблемах с конфигурационными параметрами или файлами.
 */
class ConfigurationException : public RTEMSException {
public:
    /**
     * @brief Конструктор с сообщением об ошибке
     * @param message Текстовое сообщение, описывающее ошибку конфигурации
     */
    explicit ConfigurationException(const std::string& message) 
        : RTEMSException("Configuration error: " + message) {}
};

/**
 * @class DataSourceException
 * @brief Исключение, возникающее при ошибках источника данных
 *
 * Генерируется при проблемах с получением или обработкой данных из внешних источников.
 */
class DataSourceException : public RTEMSException {
public:
    /**
     * @brief Конструктор с сообщением об ошибке
     * @param message Текстовое сообщение, описывающее ошибку источника данных
     */
    explicit DataSourceException(const std::string& message) 
        : RTEMSException("Data source error: " + message) {}
};

/**
 * @class StorageException
 * @brief Исключение, возникающее при ошибках хранения данных
 *
 * Генерируется при проблемах с сохранением или загрузкой данных.
 */
class StorageException : public RTEMSException {
public:
    /**
     * @brief Конструктор с сообщением об ошибке
     * @param message Текстовое сообщение, описывающее ошибку хранения
     */
    explicit StorageException(const std::string& message) 
        : RTEMSException("Storage error: " + message) {}
};

/**
 * @class SimulationException
 * @brief Исключение, возникающее при ошибках моделирования
 *
 * Генерируется при проблемах в процессе моделирования электромеханической системы.
 */
class SimulationException : public RTEMSException {
public:
    /**
     * @brief Конструктор с сообщением об ошибке
     * @param message Текстовое сообщение, описывающее ошибку моделирования
     */
    explicit SimulationException(const std::string& message) 
        : RTEMSException("Simulation error: " + message) {}
};

/**
 * @class ComponentNotFoundException
 * @brief Исключение, возникающее при отсутствии компонента
 *
 * Генерируется, когда запрашиваемый компонент системы не найден.
 */
class ComponentNotFoundException : public RTEMSException {
public:
    /**
     * @brief Конструктор с именем компонента
     * @param component_name Имя компонента, который не был найден
     */
    explicit ComponentNotFoundException(const std::string& component_name) 
        : RTEMSException("Component not found: " + component_name) {}
};

/**
 * @class NodeNotFoundException
 * @brief Исключение, возникающее при отсутствии узла в сети
 *
 * Генерируется, когда запрашиваемый узел электрической сети не найден.
 */
class NodeNotFoundException : public RTEMSException {
public:
    /**
     * @brief Конструктор с идентификатором узла
     * @param node_id Идентификатор узла, который не был найден
     */
    explicit NodeNotFoundException(int node_id) 
        : RTEMSException("Node not found: " + std::to_string(node_id)) {}
};

/**
 * @class GeneratorNotFoundException
 * @brief Исключение, возникающее при отсутствии генератора
 *
 * Генерируется, когда запрашиваемый генератор не найден.
 */
class GeneratorNotFoundException : public RTEMSException {
public:
    /**
     * @brief Конструктор с идентификатором генератора
     * @param generator_id Идентификатор генератора, который не был найден
     */
    explicit GeneratorNotFoundException(int generator_id) 
        : RTEMSException("Generator not found: " + std::to_string(generator_id)) {}
};

} // namespace common
} // namespace rtems 