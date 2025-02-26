#pragma once

#include "imonitor.hpp"
#include "../common/config.hpp"
#include <map>
#include <mutex>

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
 * @class SubsystemDetector
 * @brief Класс для обнаружения и анализа колебательных подсистем
 *
 * Реализует интерфейс IMonitor и выполняет:
 * - выделение когерентных групп генераторов в подсистемы
 * - расчет параметров подсистем
 * - оценку опасности и близости к нарушению устойчивости
 */
class SubsystemDetector : public IMonitor {
public:
    /**
     * @brief Конструктор детектора подсистем
     * @param config Конфигурация с параметрами детектора
     */
    explicit SubsystemDetector(const common::Config& config);
    
    /**
     * @brief Деструктор
     */
    ~SubsystemDetector() override = default;
    
    // Реализация методов интерфейса IMonitor
    /**
     * @brief Инициализация детектора с указанным источником данных
     * @param data_source Указатель на источник данных
     * @throw InitializationException в случае ошибки инициализации
     */
    void initialize(datasource::IDataSourcePtr data_source) override;
    
    /**
     * @brief Обновление данных и повторное обнаружение подсистем
     * @throw DataSourceException в случае ошибки получения данных
     */
    void update() override;
    
    /**
     * @brief Получение списка обнаруженных подсистем
     * @return Вектор обнаруженных подсистем
     */
    std::vector<common::Subsystem> get_subsystems() const override;
    
    /**
     * @brief Проверка превышения порога по углу между подсистемами
     * @return true если угол между подсистемами превышает порог, false в противном случае
     */
    bool is_angle_threshold_exceeded() const override;
    
    /**
     * @brief Получение максимальной разницы углов между подсистемами
     * @return Максимальная разница углов в градусах
     */
    double get_max_angle_difference() const override;
    
    /**
     * @brief Получение индексов подсистем с максимальной разницей углов
     * @return Пара индексов критических подсистем
     */
    std::pair<int, int> get_critical_subsystem_indices() const override;
    
    /**
     * @brief Получение подсистемы по индексу
     * @param index Индекс подсистемы
     * @return Структура подсистемы
     * @throw ComponentNotFoundException если подсистема с указанным индексом не найдена
     */
    common::Subsystem get_subsystem(int index) const override;
    
    /**
     * @brief Проверка устойчивости системы
     * @return true если система устойчива, false в противном случае
     */
    bool is_system_stable() const override;
    
private:
    /**
     * @brief Конфигурация детектора подсистем
     */
    common::Config config_;
    
    /**
     * @brief Источник данных для мониторинга
     */
    datasource::IDataSourcePtr data_source_;
    
    /**
     * @brief Флаг инициализации
     */
    bool initialized_ = false;
    
    /**
     * @brief Пороговое значение угла для определения критической ситуации
     */
    double angle_threshold_;
    
    /**
     * @brief Пороговое значение частоты для определения устойчивости
     */
    double frequency_threshold_;
    
    /**
     * @brief Список обнаруженных подсистем
     */
    std::vector<common::Subsystem> subsystems_;
    
    /**
     * @brief Максимальная разница углов между подсистемами
     */
    double max_angle_difference_ = 0.0;
    
    /**
     * @brief Индексы подсистем с максимальной разницей углов
     */
    std::pair<int, int> critical_subsystem_indices_ = {-1, -1};
    
    /**
     * @brief Мьютекс для обеспечения потокобезопасности
     */
    mutable std::mutex data_mutex_;
    
    /**
     * @brief Метод для обнаружения когерентных групп генераторов и формирования подсистем
     */
    void detect_subsystems();
    
    /**
     * @brief Метод для расчета параметров каждой подсистемы
     * 
     * Вычисляет средний угол, среднюю частоту и кинетическую энергию для каждой подсистемы.
     */
    void calculate_subsystem_parameters();
    
    /**
     * @brief Метод для расчета кинетической энергии подсистемы
     * @param subsystem Подсистема, для которой выполняется расчет
     * @return Кинетическая энергия в джоулях
     */
    double calculate_kinetic_energy(const common::Subsystem& subsystem);
};

} // namespace monitoring
} // namespace rtems 