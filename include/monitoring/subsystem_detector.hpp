#pragma once

#include "imonitor.hpp"
#include "../common/config.hpp"
#include <map>
#include <mutex>

namespace rtems {
namespace monitoring {

// Класс детектора подсистем
class SubsystemDetector : public IMonitor {
public:
    // Конструктор
    explicit SubsystemDetector(const common::Config& config);
    
    // Деструктор
    ~SubsystemDetector() override = default;
    
    // Реализация методов интерфейса IMonitor
    void initialize(datasource::IDataSourcePtr data_source) override;
    void update() override;
    std::vector<common::Subsystem> get_subsystems() const override;
    bool is_angle_threshold_exceeded() const override;
    double get_max_angle_difference() const override;
    std::pair<int, int> get_critical_subsystem_indices() const override;
    common::Subsystem get_subsystem(int index) const override;
    bool is_system_stable() const override;
    
private:
    // Конфигурация
    common::Config config_;
    
    // Источник данных
    datasource::IDataSourcePtr data_source_;
    
    // Флаг инициализации
    bool initialized_ = false;
    
    // Порог угла для активации
    double angle_threshold_;
    
    // Порог частоты для определения устойчивости
    double frequency_threshold_;
    
    // Список подсистем
    std::vector<common::Subsystem> subsystems_;
    
    // Максимальная разница углов между подсистемами
    double max_angle_difference_ = 0.0;
    
    // Индексы критических подсистем
    std::pair<int, int> critical_subsystem_indices_ = {-1, -1};
    
    // Мьютекс для потокобезопасности
    mutable std::mutex data_mutex_;
    
    // Метод для определения подсистем
    void detect_subsystems();
    
    // Метод для вычисления параметров подсистем
    void calculate_subsystem_parameters();
    
    // Метод для вычисления кинетической энергии подсистемы
    double calculate_kinetic_energy(const common::Subsystem& subsystem);
};

} // namespace monitoring
} // namespace rtems 