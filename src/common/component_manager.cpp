/**
 * @file component_manager.cpp
 * @brief Реализация менеджера компонентов системы RTEMS
 * 
 * @author RTEMS Team
 * @date 2023
 */

#include "common/component_manager.hpp"
#include "common/logger.hpp"

namespace rtems {
namespace common {

// Статический метод для доступа к экземпляру синглтона
ComponentManager& ComponentManager::instance() {
    static ComponentManager instance;
    return instance;
}

// Конструктор
ComponentManager::ComponentManager() : m_initialized(false) {
    LOG_INFO("ComponentManager создан");
}

// Инициализация менеджера компонентов
bool ComponentManager::initialize(const nlohmann::json& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_initialized) {
        LOG_WARNING("ComponentManager уже инициализирован");
        return true;
    }
    
    try {
        LOG_INFO("Инициализация ComponentManager");
        
        // Дополнительные действия по инициализации можно добавить здесь
        // Например, загрузка предопределенных компонентов из конфигурации
        
        if (config.contains("components") && config["components"].is_array()) {
            for (const auto& componentConfig : config["components"]) {
                if (!componentConfig.contains("type") || !componentConfig["type"].is_string()) {
                    LOG_ERROR("Некорректная конфигурация компонента: отсутствует или некорректный тип");
                    continue;
                }
                
                std::string typeName = componentConfig["type"];
                ComponentPtr component = createAndAddComponentByName(typeName, componentConfig);
                
                if (!component) {
                    LOG_ERROR("Не удалось создать компонент типа: {}", typeName);
                }
            }
        }
        
        m_initialized = true;
        LOG_INFO("ComponentManager успешно инициализирован");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Ошибка при инициализации ComponentManager: {}", e.what());
        return false;
    }
}

// Деинициализация и освобождение ресурсов
void ComponentManager::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        LOG_WARNING("ComponentManager не был инициализирован");
        return;
    }
    
    LOG_INFO("Остановка всех компонентов");
    stopAllComponents();
    
    LOG_INFO("Удаление всех компонентов");
    m_components.clear();
    
    m_initialized = false;
    LOG_INFO("ComponentManager успешно деинициализирован");
}

// Добавление компонента
bool ComponentManager::addComponent(ComponentPtr component) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!component) {
        LOG_ERROR("Попытка добавить nullptr компонент");
        return false;
    }
    
    const ComponentId& componentId = component->getId();
    
    if (m_components.find(componentId) != m_components.end()) {
        LOG_ERROR("Компонент с ID: {} уже существует", componentId);
        return false;
    }
    
    m_components[componentId] = component;
    LOG_INFO("Добавлен компонент с ID: {}, тип: {} ({})", 
        componentId, 
        ComponentFactory::instance().getComponentTypeName(component->getType()),
        static_cast<int>(component->getType()));
    
    return true;
}

// Создание и добавление компонента
ComponentPtr ComponentManager::createAndAddComponent(ComponentType componentType, const nlohmann::json& config) {
    ComponentPtr component = ComponentFactory::instance().createComponent(componentType, config);
    
    if (!component) {
        return nullptr;
    }
    
    if (!addComponent(component)) {
        return nullptr;
    }
    
    return component;
}

// Создание и добавление компонента по имени типа
ComponentPtr ComponentManager::createAndAddComponentByName(const std::string& typeName, const nlohmann::json& config) {
    ComponentPtr component = ComponentFactory::instance().createComponentByName(typeName, config);
    
    if (!component) {
        return nullptr;
    }
    
    if (!addComponent(component)) {
        return nullptr;
    }
    
    return component;
}

// Получение компонента по идентификатору
ComponentPtr ComponentManager::getComponent(const ComponentId& componentId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_components.find(componentId);
    if (it == m_components.end()) {
        LOG_DEBUG("Компонент с ID: {} не найден", componentId);
        return nullptr;
    }
    
    return it->second;
}

// Получение компонентов по типу
std::vector<ComponentPtr> ComponentManager::getComponentsByType(ComponentType componentType) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<ComponentPtr> result;
    
    for (const auto& pair : m_components) {
        if (pair.second->getType() == componentType) {
            result.push_back(pair.second);
        }
    }
    
    LOG_DEBUG("Найдено {} компонентов типа: {} ({})", 
        result.size(), 
        ComponentFactory::instance().getComponentTypeName(componentType),
        static_cast<int>(componentType));
    
    return result;
}

// Удаление компонента
bool ComponentManager::removeComponent(const ComponentId& componentId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_components.find(componentId);
    if (it == m_components.end()) {
        LOG_ERROR("Попытка удалить несуществующий компонент с ID: {}", componentId);
        return false;
    }
    
    ComponentPtr component = it->second;
    
    // Остановка компонента, если он запущен
    if (component->isRunning()) {
        LOG_INFO("Остановка компонента с ID: {} перед удалением", componentId);
        component->stop();
    }
    
    m_components.erase(it);
    LOG_INFO("Удален компонент с ID: {}", componentId);
    
    return true;
}

// Запуск всех компонентов
bool ComponentManager::startAllComponents() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    bool allStarted = true;
    
    LOG_INFO("Запуск всех компонентов ({})", m_components.size());
    
    for (const auto& pair : m_components) {
        ComponentPtr component = pair.second;
        
        if (component->isRunning()) {
            LOG_DEBUG("Компонент с ID: {} уже запущен", component->getId());
            continue;
        }
        
        if (!component->isInitialized()) {
            LOG_WARNING("Компонент с ID: {} не инициализирован, пропускаем запуск", component->getId());
            allStarted = false;
            continue;
        }
        
        LOG_INFO("Запуск компонента с ID: {}", component->getId());
        if (!component->start()) {
            LOG_ERROR("Не удалось запустить компонент с ID: {}", component->getId());
            allStarted = false;
        }
    }
    
    LOG_INFO("Запуск всех компонентов {}",
        allStarted ? "успешно завершен" : "завершен с ошибками");
    
    return allStarted;
}

// Остановка всех компонентов
bool ComponentManager::stopAllComponents() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    bool allStopped = true;
    
    LOG_INFO("Остановка всех компонентов ({})", m_components.size());
    
    for (const auto& pair : m_components) {
        ComponentPtr component = pair.second;
        
        if (!component->isRunning()) {
            LOG_DEBUG("Компонент с ID: {} уже остановлен", component->getId());
            continue;
        }
        
        LOG_INFO("Остановка компонента с ID: {}", component->getId());
        if (!component->stop()) {
            LOG_ERROR("Не удалось остановить компонент с ID: {}", component->getId());
            allStopped = false;
        }
    }
    
    LOG_INFO("Остановка всех компонентов {}",
        allStopped ? "успешно завершена" : "завершена с ошибками");
    
    return allStopped;
}

// Получение количества компонентов
size_t ComponentManager::getComponentCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_components.size();
}

// Проверка инициализации
bool ComponentManager::isInitialized() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_initialized;
}

} // namespace common
} // namespace rtems 