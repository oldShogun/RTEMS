#pragma once

#include "ipredictor.hpp"
#include "../common/config.hpp"
#include <mutex>
#include <atomic>
#include <chrono>

namespace rtems {
namespace prediction {

// Класс быстрого предиктора
class FastPredictor : public IPredictor {
public:
    // Конструктор
    explicit FastPredictor(const common::Config& config);
    
    // Деструктор
    ~FastPredictor() override;
    
    // Реализация методов интерфейса IPredictor
    void initialize(monitoring::IMonitorPtr monitor) override;
    common::PredictionResult predict(double time_horizon) override;
    common::PredictionResult get_last_result() const override;
    bool is_prediction_running() const override;
    void stop_prediction() override;
    std::string get_type() const override;
    double get_last_execution_time() const override;
    
private:
    // Конфигурация
    common::Config config_;
    
    // Монитор
    monitoring::IMonitorPtr monitor_;
    
    // Флаг инициализации
    bool initialized_ = false;
    
    // Флаг запуска прогнозирования
    std::atomic<bool> prediction_running_{false};
    
    // Последний результат прогнозирования
    common::PredictionResult last_result_;
    
    // Шаг интегрирования
    double time_step_;
    
    // Мьютекс для потокобезопасности
    mutable std::mutex data_mutex_;
    
    // Время выполнения последнего прогнозирования в миллисекундах
    double last_execution_time_ms_ = 0.0;
    
    // Простое прогнозирование с использованием линейной экстраполяции
    common::PredictionResult predict_linear_extrapolation(
        const std::vector<common::Subsystem>& subsystems,
        double time_horizon);
    
    // Проверка устойчивости системы
    bool check_stability(
        const std::vector<common::Subsystem>& subsystems,
        double time_horizon,
        double& time_to_instability);
};

} // namespace prediction
} // namespace rtems 