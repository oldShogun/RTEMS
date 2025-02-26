#pragma once

#include <memory>
#include "../common/types.hpp"
#include "../monitoring/imonitor.hpp"

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
 * @class IPredictor
 * @brief Интерфейс предиктора для прогнозирования состояний системы
 *
 * Определяет общий интерфейс для всех алгоритмов прогнозирования,
 * позволяющих предсказывать будущее состояние электромеханической системы.
 */
class IPredictor {
public:
    /**
     * @brief Виртуальный деструктор
     */
    virtual ~IPredictor() = default;
    
    /**
     * @brief Инициализация предиктора
     * @param monitor Указатель на монитор состояния системы
     * @throw InitializationException в случае ошибки инициализации
     */
    virtual void initialize(monitoring::IMonitorPtr monitor) = 0;
    
    /**
     * @brief Запуск прогнозирования на указанный горизонт времени
     * @param time_horizon Горизонт прогнозирования в секундах
     * @return Результат прогнозирования (структура PredictionResult)
     * @throw SimulationException в случае ошибки моделирования
     */
    virtual common::PredictionResult predict(double time_horizon) = 0;
    
    /**
     * @brief Получение последнего результата прогнозирования
     * @return Последний результат прогнозирования
     */
    virtual common::PredictionResult get_last_result() const = 0;
    
    /**
     * @brief Проверка состояния прогнозирования
     * @return true если прогнозирование выполняется, false в противном случае
     */
    virtual bool is_prediction_running() const = 0;
    
    /**
     * @brief Остановка текущего процесса прогнозирования
     */
    virtual void stop_prediction() = 0;
    
    /**
     * @brief Получение типа предиктора
     * @return Строка с типом предиктора
     */
    virtual std::string get_type() const = 0;
    
    /**
     * @brief Получение времени выполнения последнего прогнозирования 
     * @return Время выполнения в миллисекундах
     */
    virtual double get_last_execution_time() const = 0;
};

/**
 * @typedef IPredictorPtr
 * @brief Тип умного указателя на интерфейс предиктора
 */
using IPredictorPtr = std::shared_ptr<IPredictor>;

} // namespace prediction
} // namespace rtems 