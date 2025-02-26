/**
 * @file component_factory.cpp
 * @brief Реализация фабрики компонентов системы RTEMS
 * 
 * @author RTEMS Team
 * @date 2023
 */

#include "common/component_factory.hpp"
#include "common/logger.hpp"

namespace rtems {
namespace common {

// Статический метод для доступа к экземпляру синглтона
ComponentFactory& ComponentFactory::instance() {
    static ComponentFactory instance;
    return instance;
}

// Конструктор
ComponentFactory::ComponentFactory() {
    LOG_INFO("ComponentFactory создана");
}

// Регистрация создателя компонента
bool ComponentFactory::registerCreator(ComponentType componentType, const std::string& typeName, CreatorFunction creator) {
    if (m_creators.find(componentType) != m_creators.end()) {
        LOG_WARNING("Попытка перерегистрации создателя компонента типа: {}", static_cast<int>(componentType));
        return false;
    }
    
    ComponentTypeInfo typeInfo;
    typeInfo.typeName = typeName;
    typeInfo.creator = creator;
    
    m_creators[componentType] = typeInfo;
    m_typeNameMap[typeName] = componentType;
    
    LOG_INFO("Зарегистрирован создатель для компонента типа: {} ({})", typeName, static_cast<int>(componentType));
    return true;
}

// Создание компонента по типу
ComponentPtr ComponentFactory::createComponent(ComponentType componentType, const nlohmann::json& config) {
    auto it = m_creators.find(componentType);
    if (it == m_creators.end()) {
        LOG_ERROR("Не найден создатель для компонента типа: {}", static_cast<int>(componentType));
        return nullptr;
    }
    
    try {
        ComponentPtr component = it->second.creator();
        if (!component) {
            LOG_ERROR("Не удалось создать компонент типа: {} ({})", 
                it->second.typeName, static_cast<int>(componentType));
            return nullptr;
        }
        
        if (!config.empty() && !component->initialize(config)) {
            LOG_ERROR("Не удалось инициализировать компонент типа: {} ({})", 
                it->second.typeName, static_cast<int>(componentType));
            return nullptr;
        }
        
        LOG_INFO("Создан компонент типа: {} ({}) с ID: {}", 
            it->second.typeName, static_cast<int>(componentType), component->getId());
        return component;
    } catch (const std::exception& e) {
        LOG_ERROR("Исключение при создании компонента типа: {} ({}): {}", 
            it->second.typeName, static_cast<int>(componentType), e.what());
        return nullptr;
    }
}

// Создание компонента по имени типа
ComponentPtr ComponentFactory::createComponentByName(const std::string& typeName, const nlohmann::json& config) {
    auto it = m_typeNameMap.find(typeName);
    if (it == m_typeNameMap.end()) {
        LOG_ERROR("Не найден тип компонента с именем: {}", typeName);
        return nullptr;
    }
    
    return createComponent(it->second, config);
}

// Получение имени типа компонента
std::string ComponentFactory::getComponentTypeName(ComponentType componentType) const {
    auto it = m_creators.find(componentType);
    if (it == m_creators.end()) {
        return "UNKNOWN";
    }
    
    return it->second.typeName;
}

// Получение типа компонента по имени
ComponentType ComponentFactory::getComponentTypeByName(const std::string& typeName) const {
    auto it = m_typeNameMap.find(typeName);
    if (it == m_typeNameMap.end()) {
        return ComponentType::UNDEFINED;
    }
    
    return it->second;
}

} // namespace common
} // namespace rtems 