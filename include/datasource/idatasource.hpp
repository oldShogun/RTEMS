#pragma once

#include <string>
#include <vector>
#include <memory>
#include "../common/types.hpp"

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
 * @class IDataSource
 * @brief Интерфейс источника данных
 *
 * Определяет общий интерфейс для всех источников данных в системе RTEMS,
 * таких как PMU, SCADA, файлы данных или симуляции.
 */
class IDataSource {
public:
    /**
     * @brief Виртуальный деструктор
     */
    virtual ~IDataSource() = default;
    
    /**
     * @brief Инициализация источника данных
     * @throw DataSourceException в случае ошибки инициализации
     */
    virtual void initialize() = 0;
    
    /**
     * @brief Проверка соединения с источником данных
     * @return true если соединение установлено, false в противном случае
     */
    virtual bool is_connected() const = 0;
    
    /**
     * @brief Получение имени источника данных
     * @return Строка с именем источника данных
     */
    virtual std::string get_name() const = 0;
    
    /**
     * @brief Получение списка доступных узлов
     * @return Вектор структур узлов
     */
    virtual std::vector<common::Node> get_nodes() const = 0;
    
    /**
     * @brief Получение списка доступных ветвей
     * @return Вектор структур ветвей
     */
    virtual std::vector<common::Branch> get_branches() const = 0;
    
    /**
     * @brief Получение списка доступных генераторов
     * @return Вектор структур генераторов
     */
    virtual std::vector<common::Generator> get_generators() const = 0;
    
    /**
     * @brief Получение списка доступных нагрузок
     * @return Вектор структур нагрузок
     */
    virtual std::vector<common::Load> get_loads() const = 0;
    
    /**
     * @brief Обновление данных из источника
     * @throw DataSourceException в случае ошибки получения данных
     */
    virtual void update() = 0;
    
    /**
     * @brief Получение угла ротора по идентификатору генератора
     * @param generator_id Идентификатор генератора
     * @return Угол ротора в градусах
     * @throw GeneratorNotFoundException если генератор не найден
     */
    virtual double get_generator_angle(common::GeneratorId generator_id) const = 0;
    
    /**
     * @brief Получение частоты по идентификатору генератора
     * @param generator_id Идентификатор генератора
     * @return Частота в Гц
     * @throw GeneratorNotFoundException если генератор не найден
     */
    virtual double get_generator_frequency(common::GeneratorId generator_id) const = 0;
    
    /**
     * @brief Получение мощности по идентификатору генератора
     * @param generator_id Идентификатор генератора
     * @return Активная мощность в МВт
     * @throw GeneratorNotFoundException если генератор не найден
     */
    virtual double get_generator_power(common::GeneratorId generator_id) const = 0;
    
    /**
     * @brief Получение напряжения по идентификатору узла
     * @param node_id Идентификатор узла
     * @return Напряжение в относительных единицах
     * @throw NodeNotFoundException если узел не найден
     */
    virtual double get_node_voltage(common::NodeId node_id) const = 0;
    
    /**
     * @brief Получение угла напряжения по идентификатору узла
     * @param node_id Идентификатор узла
     * @return Угол напряжения в градусах
     * @throw NodeNotFoundException если узел не найден
     */
    virtual double get_node_angle(common::NodeId node_id) const = 0;
};

/**
 * @typedef IDataSourcePtr
 * @brief Тип умного указателя на интерфейс источника данных
 */
using IDataSourcePtr = std::shared_ptr<IDataSource>;

} // namespace datasource
} // namespace rtems 