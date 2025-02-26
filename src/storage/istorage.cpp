/**
 * @file istorage.cpp
 * @brief Реализация фабричного метода для интерфейса хранилища данных
 * 
 * @author RTEMS Team
 * @date 2023
 */

#include "../../include/storage/istorage.hpp"
#include "../../include/storage/memory_storage.hpp"
#include "../../include/common/logger.hpp"

namespace rtems {
namespace storage {

std::unique_ptr<IStorage> IStorage::create(const nlohmann::json& config) {
    std::string storageType;
    try {
        storageType = config.value("type", "memory");
    } catch (const nlohmann::json::exception& e) {
        LOG_ERROR("IStorage::create - Failed to parse storage type: " + std::string(e.what()));
        throw rtems::common::InitializationException("Failed to parse storage configuration");
    }

    // Логируем создание хранилища
    LOG_INFO("IStorage::create - Creating " + storageType + " storage");
    
    // Создаем хранилище в зависимости от указанного типа
    if (storageType == "memory") {
        auto storage = std::make_unique<MemoryStorage>();
        if (!storage->initialize(config)) {
            LOG_ERROR("IStorage::create - Failed to initialize memory storage");
            throw rtems::common::InitializationException("Failed to initialize memory storage");
        }
        LOG_INFO("IStorage::create - Memory storage created successfully");
        return storage;
    }
    // В будущем здесь можно добавить другие типы хранилищ
    // else if (storageType == "file") { ... }
    // else if (storageType == "database") { ... }
    
    // Если тип хранилища не поддерживается
    LOG_ERROR("IStorage::create - Unknown storage type: " + storageType);
    throw rtems::common::InitializationException("Unknown storage type: " + storageType);
}

} // namespace storage
} // namespace rtems 