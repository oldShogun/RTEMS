#include "../../include/prediction/fast_predictor.hpp"
#include "../../include/common/exceptions.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace rtems {
namespace prediction {

FastPredictor::FastPredictor(const common::Config& config)
    : config_(config) {
    // Загружаем параметры из конфигурации
    time_step_ = config_.get_double("prediction.time_step", 0.01);
}

FastPredictor::~FastPredictor() {
    // Останавливаем прогнозирование, если оно запущено
    stop_prediction();
}

void FastPredictor::initialize(monitoring::IMonitorPtr monitor) {
    if (!monitor) {
        throw common::InitializationException("Monitor is nullptr");
    }
    
    monitor_ = monitor;
    initialized_ = true;
}

common::PredictionResult FastPredictor::predict(double time_horizon) {
    if (!initialized_) {
        throw common::InitializationException("FastPredictor not initialized");
    }
    
    // Устанавливаем флаг запуска прогнозирования
    prediction_running_ = true;
    
    // Запоминаем время начала прогнозирования
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Получаем текущие подсистемы
    auto subsystems = monitor_->get_subsystems();
    
    // Выполняем прогнозирование
    common::PredictionResult result = predict_linear_extrapolation(subsystems, time_horizon);
    
    // Запоминаем время окончания прогнозирования
    auto end_time = std::chrono::high_resolution_clock::now();
    
    // Вычисляем время выполнения прогнозирования
    last_execution_time_ms_ = 
        std::chrono::duration<double, std::milli>(end_time - start_time).count();
    
    // Сохраняем результат
    {
        std::lock_guard<std::mutex> lock(data_mutex_);
        last_result_ = result;
    }
    
    // Сбрасываем флаг запуска прогнозирования
    prediction_running_ = false;
    
    return result;
}

common::PredictionResult FastPredictor::get_last_result() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return last_result_;
}

bool FastPredictor::is_prediction_running() const {
    return prediction_running_;
}

void FastPredictor::stop_prediction() {
    prediction_running_ = false;
}

std::string FastPredictor::get_type() const {
    return "FastPredictor";
}

double FastPredictor::get_last_execution_time() const {
    return last_execution_time_ms_;
}

common::PredictionResult FastPredictor::predict_linear_extrapolation(
    const std::vector<common::Subsystem>& subsystems,
    double time_horizon) {
    
    common::PredictionResult result;
    
    // Если меньше 2 подсистем, то считаем систему устойчивой
    if (subsystems.size() < 2) {
        result.is_stable = true;
        result.time_to_instability = -1.0;
        result.subsystems = subsystems;
        return result;
    }
    
    // Выполняем линейную экстраполяцию
    std::vector<common::Subsystem> predicted_subsystems = subsystems;
    
    // Получаем индексы критических подсистем
    auto [i1, i2] = monitor_->get_critical_subsystem_indices();
    
    if (i1 >= 0 && i2 >= 0 && 
        i1 < static_cast<int>(subsystems.size()) && 
        i2 < static_cast<int>(subsystems.size())) {
        
        // Вычисляем скорость изменения углов для обеих подсистем
        // В реальном MVP здесь нужно использовать историю измерений для более точной оценки
        // Для демонстрации используем фиксированные значения на основе разницы частот
        double freq_diff1 = subsystems[i1].avg_frequency - 50.0;
        double freq_diff2 = subsystems[i2].avg_frequency - 50.0;
        
        // Скорость изменения угла пропорциональна разнице частот (в градусах в секунду)
        double angle_rate1 = freq_diff1 * 360.0;
        double angle_rate2 = freq_diff2 * 360.0;
        
        // Экстраполируем углы
        for (double t = 0; t <= time_horizon; t += time_step_) {
            predicted_subsystems[i1].avg_angle += angle_rate1 * time_step_;
            predicted_subsystems[i2].avg_angle += angle_rate2 * time_step_;
            
            // Проверяем устойчивость на каждом шаге
            double time_to_instability;
            bool stable = check_stability(predicted_subsystems, t, time_to_instability);
            
            if (!stable) {
                result.is_stable = false;
                result.time_to_instability = time_to_instability;
                result.subsystems = predicted_subsystems;
                
                // Формируем базовые управляющие воздействия для демонстрации
                if (std::abs(angle_rate1) > std::abs(angle_rate2)) {
                    // Первая подсистема имеет большую скорость изменения угла
                    for (auto generator_id : subsystems[i1].generators) {
                        common::ControlAction action(
                            common::ControlActionType::POWER_REDUCTION,
                            generator_id, // ID генератора
                            20.0,        // Снижение мощности на 20%
                            1,           // Высокий приоритет
                            100.0        // Ожидаемый эффект
                        );
                        result.suggested_actions.push_back(action);
                    }
                } else {
                    // Вторая подсистема имеет большую скорость изменения угла
                    for (auto generator_id : subsystems[i2].generators) {
                        common::ControlAction action(
                            common::ControlActionType::POWER_REDUCTION,
                            generator_id, // ID генератора
                            20.0,        // Снижение мощности на 20%
                            1,           // Высокий приоритет
                            100.0        // Ожидаемый эффект
                        );
                        result.suggested_actions.push_back(action);
                    }
                }
                
                return result;
            }
        }
    }
    
    // Если мы дошли до конца времени моделирования, то система устойчива
    result.is_stable = true;
    result.time_to_instability = -1.0;
    result.subsystems = predicted_subsystems;
    
    return result;
}

bool FastPredictor::check_stability(
    const std::vector<common::Subsystem>& subsystems,
    double current_time,
    double& time_to_instability) {
    
    // В MVP используем упрощенный критерий устойчивости:
    // Если разница углов между любыми двумя подсистемами превышает 180 градусов,
    // то система считается неустойчивой
    
    const double MAX_ANGLE_DIFF = 180.0;
    
    for (size_t i = 0; i < subsystems.size(); ++i) {
        for (size_t j = i + 1; j < subsystems.size(); ++j) {
            double angle_diff = std::abs(subsystems[i].avg_angle - subsystems[j].avg_angle);
            
            // Нормализация разницы углов до диапазона [0, 360)
            angle_diff = std::fmod(angle_diff, 360.0);
            if (angle_diff > 180.0) {
                angle_diff = 360.0 - angle_diff;
            }
            
            if (angle_diff > MAX_ANGLE_DIFF) {
                time_to_instability = current_time;
                return false;
            }
        }
    }
    
    time_to_instability = -1.0;
    return true;
}

} // namespace prediction
} // namespace rtems 