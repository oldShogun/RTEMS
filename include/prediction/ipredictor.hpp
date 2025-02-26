#pragma once

#include <memory>
#include "../common/types.hpp"
#include "../monitoring/imonitor.hpp"

namespace rtems {
namespace prediction {

// Интерфейс предиктора
class IPredictor {
public:
    // Виртуальный деструктор
    virtual ~IPredictor() = default;
    
    // Инициализация предиктора
    virtual void initialize(monitoring::IMonitorPtr monitor) = 0;
    
    // Запуск прогнозирования
    virtual common::PredictionResult predict(double time_horizon) = 0;
    
    // Получение последнего результата прогнозирования
    virtual common::PredictionResult get_last_result() const = 0;
    
    // Проверка прогнозирования на запуск
    virtual bool is_prediction_running() const = 0;
    
    // Остановка прогнозирования
    virtual void stop_prediction() = 0;
    
    // Получение типа предиктора
    virtual std::string get_type() const = 0;
    
    // Получение времени выполнения последнего прогнозирования в миллисекундах
    virtual double get_last_execution_time() const = 0;
};

// Тип умного указателя на интерфейс предиктора
using IPredictorPtr = std::shared_ptr<IPredictor>;

} // namespace prediction
} // namespace rtems 