#pragma once

#include <stdexcept>
#include <string>

namespace rtems {
namespace common {

// Базовое исключение системы
class RTEMSException : public std::runtime_error {
public:
    explicit RTEMSException(const std::string& message) 
        : std::runtime_error(message) {}
};

// Исключение инициализации
class InitializationException : public RTEMSException {
public:
    explicit InitializationException(const std::string& message) 
        : RTEMSException("Initialization error: " + message) {}
};

// Исключение конфигурации
class ConfigurationException : public RTEMSException {
public:
    explicit ConfigurationException(const std::string& message) 
        : RTEMSException("Configuration error: " + message) {}
};

// Исключение источника данных
class DataSourceException : public RTEMSException {
public:
    explicit DataSourceException(const std::string& message) 
        : RTEMSException("Data source error: " + message) {}
};

// Исключение хранилища данных
class StorageException : public RTEMSException {
public:
    explicit StorageException(const std::string& message) 
        : RTEMSException("Storage error: " + message) {}
};

// Исключение моделирования
class SimulationException : public RTEMSException {
public:
    explicit SimulationException(const std::string& message) 
        : RTEMSException("Simulation error: " + message) {}
};

// Исключение ненайденного компонента
class ComponentNotFoundException : public RTEMSException {
public:
    explicit ComponentNotFoundException(const std::string& component_name) 
        : RTEMSException("Component not found: " + component_name) {}
};

// Исключение ненайденного узла
class NodeNotFoundException : public RTEMSException {
public:
    explicit NodeNotFoundException(int node_id) 
        : RTEMSException("Node not found: " + std::to_string(node_id)) {}
};

// Исключение ненайденного генератора
class GeneratorNotFoundException : public RTEMSException {
public:
    explicit GeneratorNotFoundException(int generator_id) 
        : RTEMSException("Generator not found: " + std::to_string(generator_id)) {}
};

} // namespace common
} // namespace rtems 