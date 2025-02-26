#pragma once

#include "ipredictor.hpp"
#include "../common/config.hpp"
#include <mutex>
#include <atomic>
#include <chrono>

/**
 * @namespace rtems
 * @brief Корневое пространство имён для системы моделирования электромеханических систем в реальном времени
 */
namespace rtems {
/**
 * @namespace rtems::prediction
 * @brief Компоненты для прогнозирования состояния электромеханической системы
 */
namespace prediction {

/**
 * @class FastPredictor
 * @brief Быстрый предиктор для прогнозирования состояния системы
 *
 * Реализует интерфейс IPredictor с использованием упрощенных алгоритмов
 * прогнозирования, обеспечивающих высокую скорость расчета при 
 * удовлетворительной точности для оперативного принятия решений.
 */
class FastPredictor : public IPredictor {
public:
    /**
     * @brief Конструктор быстрого предиктора
     * @param config Конфигурация с параметрами предиктора
     */
    explicit FastPredictor(const common::Config& config);
    
    /**
     * @brief Деструктор
     */
    ~FastPredictor() override;
    
    // Реализация методов интерфейса IPredictor
    /**
     * @brief Инициализация предиктора с указанным монитором
     * @param monitor Указатель на монитор состояния системы
     * @throw InitializationException в случае ошибки инициализации
     */
    void initialize(monitoring::IMonitorPtr monitor) override;
    
    /**
     * @brief Запуск прогнозирования на указанный горизонт времени
     * @param time_horizon Горизонт прогнозирования в секундах
     * @return Результат прогнозирования (структура PredictionResult)
     * @throw SimulationException в случае ошибки моделирования
     */
    common::PredictionResult predict(double time_horizon) override;
    
    /**
     * @brief Получение последнего результата прогнозирования
     * @return Последний результат прогнозирования
     */
    common::PredictionResult get_last_result() const override;
    
    /**
     * @brief Проверка состояния прогнозирования
     * @return true если прогнозирование выполняется, false в противном случае
     */
    bool is_prediction_running() const override;
    
    /**
     * @brief Остановка текущего процесса прогнозирования
     */
    void stop_prediction() override;
    
    /**
     * @brief Получение типа предиктора
     * @return Строка с типом предиктора
     */
    std::string get_type() const override;
    
    /**
     * @brief Получение времени выполнения последнего прогнозирования 
     * @return Время выполнения в миллисекундах
     */
    double get_last_execution_time() const override;
    
private:
    /**
     * @brief Конфигурация предиктора
     */
    common::Config config_;
    
    /**
     * @brief Монитор состояния системы
     */
    monitoring::IMonitorPtr monitor_;
    
    /**
     * @brief Флаг инициализации
     */
    bool initialized_ = false;
    
    /**
     * @brief Флаг запуска прогнозирования
     * 
     * Атомарный флаг для безопасной работы в многопоточной среде
     */
    std::atomic<bool> prediction_running_{false};
    
    /**
     * @brief Последний результат прогнозирования
     */
    common::PredictionResult last_result_;
    
    /**
     * @brief Шаг интегрирования для прогнозирования
     */
    double time_step_;
    
    /**
     * @brief Мьютекс для обеспечения потокобезопасности
     */
    mutable std::mutex data_mutex_;
    
    /**
     * @brief Время выполнения последнего прогнозирования в миллисекундах
     */
    double last_execution_time_ms_ = 0.0;
    
    /**
     * @brief Метод прогнозирования с использованием линейной экстраполяции
     * @param subsystems Вектор подсистем для прогнозирования
     * @param time_horizon Горизонт прогнозирования в секундах
     * @return Результат прогнозирования
     */
    common::PredictionResult predict_linear_extrapolation(
        const std::vector<common::Subsystem>& subsystems,
        double time_horizon);
    
    /**
     * @brief Метод проверки устойчивости системы
     * @param subsystems Вектор подсистем для анализа
     * @param time_horizon Горизонт прогнозирования в секундах
     * @param time_to_instability Выходной параметр - время до нарушения устойчивости
     * @return true если система устойчива, false в противном случае
     */
    bool check_stability(
        const std::vector<common::Subsystem>& subsystems,
        double time_horizon,
        double& time_to_instability);
};

} // namespace prediction
} // namespace rtems 