#pragma once

#include <memory>
#include <vector>
#include "../common/types.hpp"
#include "../datasource/idatasource.hpp"

/**
 * @namespace rtems
 * @brief Корневое пространство имён для системы моделирования электромеханических систем в реальном времени
 */
namespace rtems {
/**
 * @namespace rtems::monitoring
 * @brief Компоненты для мониторинга состояния электромеханической системы
 */
namespace monitoring {

/**
 * @class IMonitor
 * @brief Интерфейс монитора для отслеживания состояния системы
 *
 * Определяет общий интерфейс для всех мониторов, 
 * отслеживающих текущее состояние электромеханической системы 
 * и выполняющих анализ её устойчивости.
 */
class IMonitor {
public:
    /**
     * @brief Виртуальный деструктор
     */
    virtual ~IMonitor() = default;
    
    /**
     * @brief Инициализация монитора с указанным источником данных
     * @param data_source Указатель на источник данных
     * @throw InitializationException в случае ошибки инициализации
     */
    virtual void initialize(datasource::IDataSourcePtr data_source) = 0;
    
    /**
     * @brief Обновление данных монитора
     * @throw DataSourceException в случае ошибки получения данных
     */
    virtual void update() = 0;
    
    /**
     * @brief Получение списка подсистем
     * @return Вектор идентифицированных подсистем
     */
    virtual std::vector<common::Subsystem> get_subsystems() const = 0;
    
    /**
     * @brief Проверка превышения порога по углу между подсистемами
     * @return true если порог превышен, false в противном случае
     */
    virtual bool is_angle_threshold_exceeded() const = 0;
    
    /**
     * @brief Получение максимальной разницы углов между подсистемами
     * @return Максимальная разница углов в градусах
     */
    virtual double get_max_angle_difference() const = 0;
    
    /**
     * @brief Получение индексов подсистем с максимальной разницей углов
     * @return Пара индексов подсистем с максимальной разницей углов
     */
    virtual std::pair<int, int> get_critical_subsystem_indices() const = 0;
    
    /**
     * @brief Получение подсистемы по индексу
     * @param index Индекс подсистемы
     * @return Структура подсистемы
     * @throw ComponentNotFoundException если подсистема с указанным индексом не найдена
     */
    virtual common::Subsystem get_subsystem(int index) const = 0;
    
    /**
     * @brief Проверка устойчивости системы
     * @return true если система устойчива, false в противном случае
     */
    virtual bool is_system_stable() const = 0;
};

/**
 * @typedef IMonitorPtr
 * @brief Тип умного указателя на интерфейс монитора
 */
using IMonitorPtr = std::shared_ptr<IMonitor>;

} // namespace monitoring
} // namespace rtems 