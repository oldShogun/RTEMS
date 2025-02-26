#pragma once

#include "idatasource.hpp"
#include "../common/config.hpp"
#include <map>
#include <mutex>

/**
 * @namespace rtems
 * @brief Корневое пространство имён для системы моделирования электромеханических систем в реальном времени
 */
namespace rtems {
/**
 * @namespace rtems::datasource
 * @brief Компоненты для работы с источниками данных
 */
namespace datasource {

/**
 * @class PMUDataSource
 * @brief Реализация источника данных PMU (Phasor Measurement Unit)
 *
 * Этот класс обеспечивает доступ к данным от устройств PMU (синхрофазоров),
 * которые измеряют параметры электрической сети в реальном времени.
 */
class PMUDataSource : public IDataSource {
public:
    /**
     * @brief Конструктор источника данных PMU
     * @param config Конфигурация для инициализации источника данных
     */
    explicit PMUDataSource(const common::Config& config);
    
    /**
     * @brief Деструктор
     */
    ~PMUDataSource() override;
    
    // Реализация методов интерфейса IDataSource
    /**
     * @brief Инициализация источника данных PMU
     * @throw DataSourceException в случае ошибки инициализации
     */
    void initialize() override;
    
    /**
     * @brief Проверка соединения с устройствами PMU
     * @return true если соединение установлено, false в противном случае
     */
    bool is_connected() const override;
    
    /**
     * @brief Получение имени источника данных
     * @return Строка с именем источника данных
     */
    std::string get_name() const override;
    
    /**
     * @brief Получение списка узлов, доступных через PMU
     * @return Вектор структур узлов
     */
    std::vector<common::Node> get_nodes() const override;
    
    /**
     * @brief Получение списка ветвей, доступных через PMU
     * @return Вектор структур ветвей
     */
    std::vector<common::Branch> get_branches() const override;
    
    /**
     * @brief Получение списка генераторов, доступных через PMU
     * @return Вектор структур генераторов
     */
    std::vector<common::Generator> get_generators() const override;
    
    /**
     * @brief Получение списка нагрузок, доступных через PMU
     * @return Вектор структур нагрузок
     */
    std::vector<common::Load> get_loads() const override;
    
    /**
     * @brief Обновление данных от устройств PMU
     * @throw DataSourceException в случае ошибки получения данных
     */
    void update() override;
    
    /**
     * @brief Получение угла ротора генератора из данных PMU
     * @param generator_id Идентификатор генератора
     * @return Угол ротора в градусах
     * @throw GeneratorNotFoundException если генератор не найден
     */
    double get_generator_angle(common::GeneratorId generator_id) const override;
    
    /**
     * @brief Получение частоты генератора из данных PMU
     * @param generator_id Идентификатор генератора
     * @return Частота в Гц
     * @throw GeneratorNotFoundException если генератор не найден
     */
    double get_generator_frequency(common::GeneratorId generator_id) const override;
    
    /**
     * @brief Получение мощности генератора из данных PMU
     * @param generator_id Идентификатор генератора
     * @return Активная мощность в МВт
     * @throw GeneratorNotFoundException если генератор не найден
     */
    double get_generator_power(common::GeneratorId generator_id) const override;
    
    /**
     * @brief Получение напряжения в узле из данных PMU
     * @param node_id Идентификатор узла
     * @return Напряжение в относительных единицах
     * @throw NodeNotFoundException если узел не найден
     */
    double get_node_voltage(common::NodeId node_id) const override;
    
    /**
     * @brief Получение угла напряжения в узле из данных PMU
     * @param node_id Идентификатор узла
     * @return Угол напряжения в градусах
     * @throw NodeNotFoundException если узел не найден
     */
    double get_node_angle(common::NodeId node_id) const override;
    
private:
    /**
     * @brief Конфигурация источника данных
     */
    common::Config config_;
    
    /**
     * @brief Флаг инициализации
     */
    bool initialized_ = false;
    
    /**
     * @brief Флаг подключения к устройствам PMU
     */
    bool connected_ = false;
    
    /**
     * @brief Имя источника данных
     */
    std::string name_ = "PMU Data Source";
    
    /**
     * @brief Кэш данных узлов, ветвей, генераторов и нагрузок
     */
    std::vector<common::Node> nodes_;        ///< Список узлов
    std::vector<common::Branch> branches_;   ///< Список ветвей
    std::vector<common::Generator> generators_; ///< Список генераторов
    std::vector<common::Load> loads_;        ///< Список нагрузок
    
    /**
     * @brief Карты для быстрого доступа к данным по идентификатору
     */
    std::map<common::NodeId, common::Node> node_map_;          ///< Карта узлов
    std::map<common::GeneratorId, common::Generator> generator_map_; ///< Карта генераторов
    
    /**
     * @brief Мьютекс для обеспечения потокобезопасности
     */
    mutable std::mutex data_mutex_;
    
    /**
     * @brief Загрузка тестовых данных для демонстрации и отладки
     */
    void load_test_data();
};

} // namespace datasource
} // namespace rtems 