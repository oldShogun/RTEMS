#pragma once

#include <string>
#include <vector>
#include <memory>
#include "../common/types.hpp"

namespace rtems {
namespace datasource {

// Интерфейс источника данных
class IDataSource {
public:
    // Виртуальный деструктор
    virtual ~IDataSource() = default;
    
    // Инициализация источника данных
    virtual void initialize() = 0;
    
    // Проверка соединения с источником данных
    virtual bool is_connected() const = 0;
    
    // Получение имени источника данных
    virtual std::string get_name() const = 0;
    
    // Получение списка доступных узлов
    virtual std::vector<common::Node> get_nodes() const = 0;
    
    // Получение списка доступных ветвей
    virtual std::vector<common::Branch> get_branches() const = 0;
    
    // Получение списка доступных генераторов
    virtual std::vector<common::Generator> get_generators() const = 0;
    
    // Получение списка доступных нагрузок
    virtual std::vector<common::Load> get_loads() const = 0;
    
    // Обновление данных
    virtual void update() = 0;
    
    // Получение угла по идентификатору генератора
    virtual double get_generator_angle(common::GeneratorId generator_id) const = 0;
    
    // Получение частоты по идентификатору генератора
    virtual double get_generator_frequency(common::GeneratorId generator_id) const = 0;
    
    // Получение мощности по идентификатору генератора
    virtual double get_generator_power(common::GeneratorId generator_id) const = 0;
    
    // Получение напряжения по идентификатору узла
    virtual double get_node_voltage(common::NodeId node_id) const = 0;
    
    // Получение угла по идентификатору узла
    virtual double get_node_angle(common::NodeId node_id) const = 0;
};

// Тип умного указателя на интерфейс источника данных
using IDataSourcePtr = std::shared_ptr<IDataSource>;

} // namespace datasource
} // namespace rtems 