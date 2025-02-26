#pragma once

#include "idatasource.hpp"
#include "../common/config.hpp"
#include <map>
#include <mutex>

namespace rtems {
namespace datasource {

// Реализация источника данных PMU
class PMUDataSource : public IDataSource {
public:
    // Конструктор
    explicit PMUDataSource(const common::Config& config);
    
    // Деструктор
    ~PMUDataSource() override;
    
    // Реализация методов интерфейса IDataSource
    void initialize() override;
    bool is_connected() const override;
    std::string get_name() const override;
    std::vector<common::Node> get_nodes() const override;
    std::vector<common::Branch> get_branches() const override;
    std::vector<common::Generator> get_generators() const override;
    std::vector<common::Load> get_loads() const override;
    void update() override;
    double get_generator_angle(common::GeneratorId generator_id) const override;
    double get_generator_frequency(common::GeneratorId generator_id) const override;
    double get_generator_power(common::GeneratorId generator_id) const override;
    double get_node_voltage(common::NodeId node_id) const override;
    double get_node_angle(common::NodeId node_id) const override;
    
private:
    // Конфигурация
    common::Config config_;
    
    // Флаг инициализации
    bool initialized_ = false;
    
    // Флаг подключения
    bool connected_ = false;
    
    // Имя источника данных
    std::string name_ = "PMU Data Source";
    
    // Данные узлов, ветвей, генераторов и нагрузок
    std::vector<common::Node> nodes_;
    std::vector<common::Branch> branches_;
    std::vector<common::Generator> generators_;
    std::vector<common::Load> loads_;
    
    // Карты для быстрого доступа к данным
    std::map<common::NodeId, common::Node> node_map_;
    std::map<common::GeneratorId, common::Generator> generator_map_;
    
    // Мьютекс для потокобезопасности
    mutable std::mutex data_mutex_;
    
    // Загрузка тестовых данных для MVP
    void load_test_data();
};

} // namespace datasource
} // namespace rtems 