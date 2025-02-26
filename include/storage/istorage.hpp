/**
 * @file istorage.hpp
 * @brief Интерфейс для компонентов хранения данных в системе RTEMS
 * @details Определяет базовый интерфейс для работы с данными, который должны реализовывать 
 *          все конкретные хранилища в системе
 * 
 * @author RTEMS Team
 * @date 2023
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <memory>
#include <functional>
#include <chrono>
#include <variant>

#include "../common/types.hpp"
#include "../common/exceptions.hpp"

namespace rtems {
namespace storage {

/**
 * @typedef TimePoint
 * @brief Тип для представления временной метки
 */
using TimePoint = std::chrono::time_point<std::chrono::system_clock>;

/**
 * @typedef Value
 * @brief Тип для представления значений различных типов в хранилище
 */
using Value = std::variant<double, int, bool, std::string>;

/**
 * @brief Интерфейс хранилища данных для системы RTEMS
 * 
 * @details Этот класс определяет интерфейс для всех хранилищ данных в системе RTEMS.
 *          Хранилище должно поддерживать операции сохранения, извлечения, обновления данных,
 *          а также поиска по ключу и запросов с фильтрацией.
 *          
 *          Интерфейс спроектирован для работы с различными типами данных (измерения PMU, 
 *          результаты расчетов, настройки и т.д.) и может быть реализован разными способами
 *          (в памяти, в файле, в базе данных и т.п.)
 */
class IStorage {
public:
    /**
     * @brief Виртуальный деструктор
     */
    virtual ~IStorage() = default;

    /**
     * @brief Инициализирует хранилище
     * 
     * @param config Конфигурация хранилища в формате JSON
     * @return true если инициализация прошла успешно
     * @return false если возникли ошибки при инициализации
     * @throws rtems::common::InitializationException при критических ошибках инициализации
     */
    virtual bool initialize(const nlohmann::json& config) = 0;

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
    virtual bool storeTimeSeries(const std::string& key, 
                                const std::vector<Value>& data,
                                const std::vector<TimePoint>& timestamps) = 0;

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
    virtual std::pair<std::vector<Value>, std::vector<TimePoint>> 
    getTimeSeries(const std::string& key, 
                  const TimePoint& startTime, 
                  const TimePoint& endTime) = 0;

    /**
     * @brief Удаляет временной ряд из хранилища
     * 
     * @param key Уникальный идентификатор временного ряда
     * @return true если данные успешно удалены
     * @return false если произошла ошибка или данные не найдены
     * @throws rtems::common::StorageException при ошибке удаления
     */
    virtual bool removeTimeSeries(const std::string& key) = 0;

    /**
     * @brief Сохраняет одиночное значение в хранилище
     * 
     * @param key Уникальный идентификатор значения
     * @param value Значение для сохранения
     * @return true если значение успешно сохранено
     * @return false если произошла ошибка при сохранении
     * @throws rtems::common::StorageException при ошибке сохранения
     */
    virtual bool storeValue(const std::string& key, const Value& value) = 0;

    /**
     * @brief Получает одиночное значение из хранилища
     * 
     * @param key Уникальный идентификатор значения
     * @return std::optional<Value> Значение, если оно найдено, или std::nullopt
     * @throws rtems::common::StorageException при ошибке извлечения данных
     */
    virtual std::optional<Value> getValue(const std::string& key) = 0;

    /**
     * @brief Сохраняет структуру данных в хранилище в формате JSON
     * 
     * @param key Уникальный идентификатор структуры
     * @param data Структура данных в формате JSON
     * @return true если структура успешно сохранена
     * @return false если произошла ошибка при сохранении
     * @throws rtems::common::StorageException при ошибке сохранения
     */
    virtual bool storeStructure(const std::string& key, const nlohmann::json& data) = 0;

    /**
     * @brief Получает структуру данных из хранилища
     * 
     * @param key Уникальный идентификатор структуры
     * @return std::optional<nlohmann::json> Структура данных, если она найдена, или std::nullopt
     * @throws rtems::common::StorageException при ошибке извлечения данных
     */
    virtual std::optional<nlohmann::json> getStructure(const std::string& key) = 0;

    /**
     * @brief Очищает хранилище, удаляя все данные
     * 
     * @return true если хранилище успешно очищено
     * @return false если произошла ошибка при очистке
     * @throws rtems::common::StorageException при ошибке очистки
     */
    virtual bool clear() = 0;

    /**
     * @brief Создает новый экземпляр хранилища
     * 
     * @param config Конфигурация хранилища в формате JSON
     * @return std::unique_ptr<IStorage> Указатель на созданное хранилище
     * @throws rtems::common::InitializationException при ошибке создания хранилища
     */
    static std::unique_ptr<IStorage> create(const nlohmann::json& config);
};

} // namespace storage
} // namespace rtems 