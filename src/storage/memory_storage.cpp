/**
 * @file memory_storage.cpp
 * @brief Реализация методов класса MemoryStorage
 * 
 * @author RTEMS Team
 * @date 2023
 */

#include "../../include/storage/memory_storage.hpp"
#include "../../include/common/logger.hpp"
#include <algorithm>

namespace rtems {
namespace storage {

bool MemoryStorage::initialize(const nlohmann::json& config) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    
    try {
        // Получаем максимальный размер временного ряда из конфигурации, если указан
        if (config.contains("max_time_series_size")) {
            m_maxTimeSeriesSize = config["max_time_series_size"].get<std::size_t>();
        }
        
        // Логируем успешную инициализацию
        LOG_INFO("MemoryStorage::initialize - Storage initialized with max time series size: " + std::to_string(m_maxTimeSeriesSize));
        
        // Отмечаем хранилище как инициализированное
        m_initialized = true;
        return true;
    } catch (const std::exception& e) {
        // Логируем ошибку инициализации
        LOG_ERROR("MemoryStorage::initialize - Initialization failed: " + std::string(e.what()));
        return false;
    }
}

bool MemoryStorage::storeTimeSeries(const std::string& key, 
                                    const std::vector<Value>& data,
                                    const std::vector<TimePoint>& timestamps) {
    // Проверяем, что хранилище инициализировано
    if (!m_initialized) {
        LOG_WARNING("MemoryStorage::storeTimeSeries - Storage not initialized");
        return false;
    }
    
    // Проверяем, что размеры векторов данных и временных меток совпадают
    if (data.size() != timestamps.size()) {
        LOG_ERROR("MemoryStorage::storeTimeSeries - Data and timestamps size mismatch: " + 
                std::to_string(data.size()) + " vs " + std::to_string(timestamps.size()));
        throw rtems::common::StorageException("Data and timestamps size mismatch");
    }
    
    try {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        
        // Создаем или заменяем временной ряд
        TimeSeriesData newData;
        newData.values = data;
        newData.timestamps = timestamps;
        
        // Если размер превышает максимальный, обрезаем
        if (data.size() > m_maxTimeSeriesSize) {
            LOG_WARNING("MemoryStorage::storeTimeSeries - Truncating data from " + 
                      std::to_string(data.size()) + " to " + std::to_string(m_maxTimeSeriesSize) + " elements");
            newData.values.resize(m_maxTimeSeriesSize);
            newData.timestamps.resize(m_maxTimeSeriesSize);
        }
        
        m_timeSeriesData[key] = std::move(newData);
        
        LOG_DEBUG("MemoryStorage::storeTimeSeries - Stored " + 
                std::to_string(m_timeSeriesData[key].values.size()) + " values for key '" + key + "'");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("MemoryStorage::storeTimeSeries - Failed to store data: " + std::string(e.what()));
        throw rtems::common::StorageException("Failed to store time series: " + std::string(e.what()));
    }
}

std::pair<std::vector<Value>, std::vector<TimePoint>> 
MemoryStorage::getTimeSeries(const std::string& key, 
                             const TimePoint& startTime, 
                             const TimePoint& endTime) {
    // Проверяем, что хранилище инициализировано
    if (!m_initialized) {
        LOG_WARNING("MemoryStorage::getTimeSeries - Storage not initialized");
        throw rtems::common::StorageException("Storage not initialized");
    }
    
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    
    // Проверяем, существует ли временной ряд с указанным ключом
    auto it = m_timeSeriesData.find(key);
    if (it == m_timeSeriesData.end()) {
        LOG_WARNING("MemoryStorage::getTimeSeries - Key '" + key + "' not found");
        return std::make_pair(std::vector<Value>(), std::vector<TimePoint>());
    }
    
    // Фильтруем данные по временному интервалу
    std::vector<Value> filteredValues;
    std::vector<TimePoint> filteredTimestamps;
    
    const auto& data = it->second;
    for (size_t i = 0; i < data.timestamps.size(); ++i) {
        if (data.timestamps[i] >= startTime && data.timestamps[i] <= endTime) {
            filteredValues.push_back(data.values[i]);
            filteredTimestamps.push_back(data.timestamps[i]);
        }
    }
    
    LOG_DEBUG("MemoryStorage::getTimeSeries - Retrieved " + 
             std::to_string(filteredValues.size()) + " values for key '" + key + "'");
    
    return std::make_pair(std::move(filteredValues), std::move(filteredTimestamps));
}

bool MemoryStorage::removeTimeSeries(const std::string& key) {
    // Проверяем, что хранилище инициализировано
    if (!m_initialized) {
        LOG_WARNING("MemoryStorage::removeTimeSeries - Storage not initialized");
        return false;
    }
    
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    
    // Удаляем временной ряд
    auto it = m_timeSeriesData.find(key);
    if (it != m_timeSeriesData.end()) {
        m_timeSeriesData.erase(it);
        LOG_DEBUG("MemoryStorage::removeTimeSeries - Removed time series with key '" + key + "'");
        return true;
    }
    
    LOG_WARNING("MemoryStorage::removeTimeSeries - Key '" + key + "' not found");
    return false;
}

bool MemoryStorage::storeValue(const std::string& key, const Value& value) {
    // Проверяем, что хранилище инициализировано
    if (!m_initialized) {
        LOG_WARNING("MemoryStorage::storeValue - Storage not initialized");
        return false;
    }
    
    try {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        
        // Сохраняем значение
        m_values[key] = value;
        
        LOG_DEBUG("MemoryStorage::storeValue - Stored value for key '" + key + "'");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("MemoryStorage::storeValue - Failed to store value: " + std::string(e.what()));
        throw rtems::common::StorageException("Failed to store value: " + std::string(e.what()));
    }
}

std::optional<Value> MemoryStorage::getValue(const std::string& key) {
    // Проверяем, что хранилище инициализировано
    if (!m_initialized) {
        LOG_WARNING("MemoryStorage::getValue - Storage not initialized");
        throw rtems::common::StorageException("Storage not initialized");
    }
    
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    
    // Получаем значение
    auto it = m_values.find(key);
    if (it != m_values.end()) {
        LOG_DEBUG("MemoryStorage::getValue - Retrieved value for key '" + key + "'");
        return it->second;
    }
    
    LOG_WARNING("MemoryStorage::getValue - Key '" + key + "' not found");
    return std::nullopt;
}

bool MemoryStorage::storeStructure(const std::string& key, const nlohmann::json& data) {
    // Проверяем, что хранилище инициализировано
    if (!m_initialized) {
        LOG_WARNING("MemoryStorage::storeStructure - Storage not initialized");
        return false;
    }
    
    try {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        
        // Сохраняем структуру JSON
        m_structures[key] = data;
        
        LOG_DEBUG("MemoryStorage::storeStructure - Stored structure for key '" + key + "'");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("MemoryStorage::storeStructure - Failed to store structure: " + std::string(e.what()));
        throw rtems::common::StorageException("Failed to store structure: " + std::string(e.what()));
    }
}

std::optional<nlohmann::json> MemoryStorage::getStructure(const std::string& key) {
    // Проверяем, что хранилище инициализировано
    if (!m_initialized) {
        LOG_WARNING("MemoryStorage::getStructure - Storage not initialized");
        throw rtems::common::StorageException("Storage not initialized");
    }
    
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    
    // Получаем структуру JSON
    auto it = m_structures.find(key);
    if (it != m_structures.end()) {
        LOG_DEBUG("MemoryStorage::getStructure - Retrieved structure for key '" + key + "'");
        return it->second;
    }
    
    LOG_WARNING("MemoryStorage::getStructure - Key '" + key + "' not found");
    return std::nullopt;
}

bool MemoryStorage::clear() {
    // Проверяем, что хранилище инициализировано
    if (!m_initialized) {
        LOG_WARNING("MemoryStorage::clear - Storage not initialized");
        return false;
    }
    
    try {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        
        // Очищаем все хранилища
        m_timeSeriesData.clear();
        m_values.clear();
        m_structures.clear();
        
        LOG_INFO("MemoryStorage::clear - Storage cleared");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("MemoryStorage::clear - Failed to clear storage: " + std::string(e.what()));
        throw rtems::common::StorageException("Failed to clear storage: " + std::string(e.what()));
    }
}

} // namespace storage
} // namespace rtems 