#pragma once

#include <memory>
#include <vector>
#include "../common/types.hpp"
#include "../datasource/idatasource.hpp"

namespace rtems {
namespace monitoring {

// Интерфейс монитора
class IMonitor {
public:
    // Виртуальный деструктор
    virtual ~IMonitor() = default;
    
    // Инициализация монитора
    virtual void initialize(datasource::IDataSourcePtr data_source) = 0;
    
    // Обновление данных монитора
    virtual void update() = 0;
    
    // Получение списка подсистем
    virtual std::vector<common::Subsystem> get_subsystems() const = 0;
    
    // Проверка превышения порога по углу
    virtual bool is_angle_threshold_exceeded() const = 0;
    
    // Получение максимальной разницы углов между подсистемами
    virtual double get_max_angle_difference() const = 0;
    
    // Получение индексов подсистем с максимальной разницей углов
    virtual std::pair<int, int> get_critical_subsystem_indices() const = 0;
    
    // Получение подсистемы по индексу
    virtual common::Subsystem get_subsystem(int index) const = 0;
    
    // Проверка устойчивости системы
    virtual bool is_system_stable() const = 0;
};

// Тип умного указателя на интерфейс монитора
using IMonitorPtr = std::shared_ptr<IMonitor>;

} // namespace monitoring
} // namespace rtems 