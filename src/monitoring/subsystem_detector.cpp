#include "../../include/monitoring/subsystem_detector.hpp"
#include "../../include/common/exceptions.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace rtems {
namespace monitoring {

SubsystemDetector::SubsystemDetector(const common::Config& config)
    : config_(config) {
    // Загружаем параметры из конфигурации
    angle_threshold_ = config_.get_double("monitoring.angle_threshold", 
                                           common::DEFAULT_ANGLE_THRESHOLD);
    frequency_threshold_ = config_.get_double("monitoring.frequency_threshold", 0.5);
}

void SubsystemDetector::initialize(datasource::IDataSourcePtr data_source) {
    if (!data_source) {
        throw common::InitializationException("Data source is nullptr");
    }
    
    data_source_ = data_source;
    initialized_ = true;
    
    // Выполняем первоначальное обновление
    update();
}

void SubsystemDetector::update() {
    if (!initialized_) {
        throw common::InitializationException("SubsystemDetector not initialized");
    }
    
    // Обновляем данные из источника
    data_source_->update();
    
    // Определяем подсистемы
    detect_subsystems();
    
    // Вычисляем параметры подсистем
    calculate_subsystem_parameters();
}

std::vector<common::Subsystem> SubsystemDetector::get_subsystems() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return subsystems_;
}

bool SubsystemDetector::is_angle_threshold_exceeded() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return max_angle_difference_ > angle_threshold_;
}

double SubsystemDetector::get_max_angle_difference() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return max_angle_difference_;
}

std::pair<int, int> SubsystemDetector::get_critical_subsystem_indices() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return critical_subsystem_indices_;
}

common::Subsystem SubsystemDetector::get_subsystem(int index) const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    if (index < 0 || index >= static_cast<int>(subsystems_.size())) {
        throw std::out_of_range("Subsystem index out of range");
    }
    return subsystems_[index];
}

bool SubsystemDetector::is_system_stable() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    // Проверяем устойчивость по частоте генераторов
    auto generators = data_source_->get_generators();
    double reference_frequency = 50.0; // Эталонная частота, Гц
    
    for (const auto& generator : generators) {
        double freq_deviation = std::abs(generator.frequency - reference_frequency);
        if (freq_deviation > frequency_threshold_) {
            return false;
        }
    }
    
    return true;
}

void SubsystemDetector::detect_subsystems() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    // Получаем список генераторов
    auto generators = data_source_->get_generators();
    
    // Очищаем текущие подсистемы
    subsystems_.clear();
    
    // Если у нас меньше 2 генераторов, то подсистем нет
    if (generators.size() < 2) {
        return;
    }
    
    // В MVP мы используем упрощенный алгоритм определения подсистем
    // Разделяем генераторы на две группы по знаку отклонения частоты от номинальной
    common::Subsystem subsystem_positive(1);
    common::Subsystem subsystem_negative(2);
    
    double reference_frequency = 50.0; // Эталонная частота, Гц
    
    for (const auto& generator : generators) {
        double freq_deviation = generator.frequency - reference_frequency;
        
        if (freq_deviation >= 0) {
            subsystem_positive.generators.push_back(generator.id);
        } else {
            subsystem_negative.generators.push_back(generator.id);
        }
    }
    
    // Добавляем подсистемы, только если в них есть генераторы
    if (!subsystem_positive.generators.empty()) {
        subsystems_.push_back(subsystem_positive);
    }
    
    if (!subsystem_negative.generators.empty()) {
        subsystems_.push_back(subsystem_negative);
    }
}

void SubsystemDetector::calculate_subsystem_parameters() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    // Если у нас меньше 2 подсистем, то критических подсистем нет
    if (subsystems_.size() < 2) {
        max_angle_difference_ = 0.0;
        critical_subsystem_indices_ = {-1, -1};
        return;
    }
    
    // Вычисляем параметры для каждой подсистемы
    for (auto& subsystem : subsystems_) {
        double total_power = 0.0;
        double weighted_angle_sum = 0.0;
        double weighted_frequency_sum = 0.0;
        
        for (auto generator_id : subsystem.generators) {
            try {
                double power = data_source_->get_generator_power(generator_id);
                double angle = data_source_->get_generator_angle(generator_id);
                double frequency = data_source_->get_generator_frequency(generator_id);
                
                total_power += power;
                weighted_angle_sum += angle * power;
                weighted_frequency_sum += frequency * power;
            } catch (const common::GeneratorNotFoundException& e) {
                std::cerr << "Warning: " << e.what() << std::endl;
                continue;
            }
        }
        
        if (total_power > 0) {
            subsystem.avg_angle = weighted_angle_sum / total_power;
            subsystem.avg_frequency = weighted_frequency_sum / total_power;
        } else {
            subsystem.avg_angle = 0.0;
            subsystem.avg_frequency = 50.0;
        }
        
        // Вычисляем кинетическую энергию подсистемы
        subsystem.kinetic_energy = calculate_kinetic_energy(subsystem);
    }
    
    // Находим пару подсистем с максимальной разницей углов
    max_angle_difference_ = 0.0;
    critical_subsystem_indices_ = {-1, -1};
    
    for (size_t i = 0; i < subsystems_.size(); ++i) {
        for (size_t j = i + 1; j < subsystems_.size(); ++j) {
            double angle_diff = std::abs(subsystems_[i].avg_angle - subsystems_[j].avg_angle);
            
            if (angle_diff > max_angle_difference_) {
                max_angle_difference_ = angle_diff;
                critical_subsystem_indices_ = {static_cast<int>(i), static_cast<int>(j)};
            }
        }
    }
}

double SubsystemDetector::calculate_kinetic_energy(const common::Subsystem& subsystem) {
    double total_energy = 0.0;
    
    for (auto generator_id : subsystem.generators) {
        try {
            auto generators = data_source_->get_generators();
            auto it = std::find_if(generators.begin(), generators.end(),
                                  [generator_id](const common::Generator& g) {
                                      return g.id == generator_id;
                                  });
            
            if (it != generators.end()) {
                // Формула для кинетической энергии: 0.5 * J * ω², где J - момент инерции, ω - угловая скорость
                double angular_velocity = 2.0 * common::PI * it->frequency;
                total_energy += 0.5 * it->inertia * angular_velocity * angular_velocity;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error calculating kinetic energy: " << e.what() << std::endl;
            continue;
        }
    }
    
    return total_energy;
}

} // namespace monitoring
} // namespace rtems 