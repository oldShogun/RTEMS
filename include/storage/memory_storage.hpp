/**
 * @file memory_storage.hpp
 * @brief Реализация хранилища данных в оперативной памяти
 * @details Хранилище данных, работающее в оперативной памяти, для быстрого доступа к данным
 * 
 * @author RTEMS Team
 * @date 2023
 */

#pragma once

#include "istorage.hpp"
#include <map>
#include <shared_mutex>

namespace rtems {
namespace storage {

/**
 * @brief Реализация хранилища данных в оперативной памяти
 * 
 * @details Класс реализует интерфейс IStorage и предоставляет хранилище данных,
 *          которое хранит все данные в оперативной памяти для быстрого доступа.
 *          Используется для кэширования и временного хранения данных.
 *          
 *          Данное хранилище поддерживает потокобезопасный доступ через механизм
 *          блокировок для чтения/записи (shared_mutex).
 */
class MemoryStorage : public IStorage {
public:
    /**
     * @brief Конструктор по умолчанию
     */
    MemoryStorage() = default;

    /**
     * @brief Деструктор
     */
    ~MemoryStorage() override = default;

    /**
     * @brief Инициализирует хранилище в памяти
     * 
     * @param config Конфигурация хранилища в формате JSON
     * @return true если инициализация прошла успешно
     * @return false если возникли ошибки при инициализации
     * @throws rtems::common::InitializationException при критических ошибках инициализации
     */
    bool initialize(const nlohmann::json& config) override;

    /**
     * @brief Сохраняет временной ряд измерений или результатов расчетов
     * 
     * @param key Уникальный идентификатор временного ряда
     * @param data Вектор значений для сохранения
     * @param timestamps Вектор временных меток, соответствующих значениям
     * @return true если данные успешно сохранены
     * @return false если произошла ошибка при сохранении
     * @throws rtems::common::StorageException при ошибке сохранения
     */
    bool storeTimeSeries(const std::string& key, 
                         const std::vector<Value>& data,
                         const std::vector<TimePoint>& timestamps) override;

    /**
     * @brief Получает временной ряд из хранилища
     * 
     * @param key Уникальный идентификатор временного ряда
     * @param startTime Начальное время выборки (включительно)
     * @param endTime Конечное время выборки (включительно)
     * @return std::pair<std::vector<Value>, std::vector<TimePoint>> 
     *         Пара векторов значений и соответствующих временных меток
     * @throws rtems::common::StorageException при ошибке извлечения данных
     */
    std::pair<std::vector<Value>, std::vector<TimePoint>> 
    getTimeSeries(const std::string& key, 
                  const TimePoint& startTime, 
                  const TimePoint& endTime) override;

    /**
     * @brief Удаляет временной ряд из хранилища
     * 
     * @param key Уникальный идентификатор временного ряда
     * @return true если данные успешно удалены
     * @return false если произошла ошибка или данные не найдены
     * @throws rtems::common::StorageException при ошибке удаления
     */
    bool removeTimeSeries(const std::string& key) override;

    /**
     * @brief Сохраняет одиночное значение в хранилище
     * 
     * @param key Уникальный идентификатор значения
     * @param value Значение для сохранения
     * @return true если значение успешно сохранено
     * @return false если произошла ошибка при сохранении
     * @throws rtems::common::StorageException при ошибке сохранения
     */
    bool storeValue(const std::string& key, const Value& value) override;

    /**
     * @brief Получает одиночное значение из хранилища
     * 
     * @param key Уникальный идентификатор значения
     * @return std::optional<Value> Значение, если оно найдено, или std::nullopt
     * @throws rtems::common::StorageException при ошибке извлечения данных
     */
    std::optional<Value> getValue(const std::string& key) override;

    /**
     * @brief Сохраняет структуру данных в хранилище в формате JSON
     * 
     * @param key Уникальный идентификатор структуры
     * @param data Структура данных в формате JSON
     * @return true если структура успешно сохранена
     * @return false если произошла ошибка при сохранении
     * @throws rtems::common::StorageException при ошибке сохранения
     */
    bool storeStructure(const std::string& key, const nlohmann::json& data) override;

    /**
     * @brief Получает структуру данных из хранилища
     * 
     * @param key Уникальный идентификатор структуры
     * @return std::optional<nlohmann::json> Структура данных, если она найдена, или std::nullopt
     * @throws rtems::common::StorageException при ошибке извлечения данных
     */
    std::optional<nlohmann::json> getStructure(const std::string& key) override;

    /**
     * @brief Очищает хранилище, удаляя все данные
     * 
     * @return true если хранилище успешно очищено
     * @return false если произошла ошибка при очистке
     * @throws rtems::common::StorageException при ошибке очистки
     */
    bool clear() override;

private:
    /**
     * @brief Структура для хранения временного ряда данных
     */
    struct TimeSeriesData {
        std::vector<Value> values;       ///< Значения временного ряда
        std::vector<TimePoint> timestamps; ///< Временные метки
    };

    std::unordered_map<std::string, TimeSeriesData> m_timeSeriesData; ///< Хранилище временных рядов
    std::unordered_map<std::string, Value> m_values;   ///< Хранилище одиночных значений
    std::unordered_map<std::string, nlohmann::json> m_structures;    ///< Хранилище структур JSON

    mutable std::shared_mutex m_mutex; ///< Мьютекс для контроля доступа к данным

    std::size_t m_maxTimeSeriesSize = 10000; ///< Максимальный размер временного ряда
    bool m_initialized = false;              ///< Флаг инициализации хранилища
};

} // namespace storage
} // namespace rtems 