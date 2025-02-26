#pragma once

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <cmath>
#include <stdexcept>

#include <nlohmann/json.hpp>

/**
 * @namespace rtems
 * @brief Корневое пространство имён для системы моделирования электромеханических систем в реальном времени
 */
namespace rtems {
/**
 * @namespace rtems::common
 * @brief Общие компоненты, используемые всеми модулями системы
 */
namespace common {

/// @name Базовые константы
/// @{
/**
 * @brief Математическая константа π
 */
constexpr double PI = 3.14159265358979323846;

/**
 * @brief Порог по углу по умолчанию в градусах
 */
constexpr double DEFAULT_ANGLE_THRESHOLD = 20.0;

/**
 * @brief Максимальная частота измерений PMU в секунду
 */
constexpr int MAX_PMU_FREQUENCY = 50;
/// @}

/// @name Определения типов идентификаторов
/// @{
/**
 * @typedef NodeId
 * @brief Тип идентификатора узла сети
 */
using NodeId = int;

/**
 * @typedef BranchId
 * @brief Тип идентификатора ветви сети
 */
using BranchId = int;

/**
 * @typedef GeneratorId
 * @brief Тип идентификатора генератора
 */
using GeneratorId = int;

/**
 * @typedef LoadId
 * @brief Тип идентификатора нагрузки
 */
using LoadId = int;

/**
 * @typedef SubsystemId
 * @brief Тип идентификатора подсистемы
 */
using SubsystemId = int;
/// @}

/**
 * @struct Node
 * @brief Структура для представления узла электрической сети
 */
struct Node {
    NodeId id;            ///< Идентификатор узла
    std::string name;     ///< Наименование узла
    double voltage;       ///< Напряжение в узле (в относительных единицах)
    double angle;         ///< Угол напряжения (в градусах)
    bool is_generator;    ///< Флаг наличия генератора в узле
    
    /**
     * @brief Конструктор структуры узла
     * @param id Идентификатор узла
     * @param name Наименование узла
     * @param voltage Напряжение в относительных единицах
     * @param angle Угол напряжения в градусах
     * @param is_generator Флаг наличия генератора
     */
    Node(NodeId id = -1, const std::string& name = "", double voltage = 0.0, 
         double angle = 0.0, bool is_generator = false)
        : id(id), name(name), voltage(voltage), angle(angle), is_generator(is_generator) {}
};

/**
 * @struct Branch
 * @brief Структура для представления ветви электрической сети
 */
struct Branch {
    BranchId id;        ///< Идентификатор ветви
    NodeId from_node;   ///< Идентификатор начального узла
    NodeId to_node;     ///< Идентификатор конечного узла
    double resistance;  ///< Активное сопротивление (в о.е.)
    double reactance;   ///< Реактивное сопротивление (в о.е.)
    double capacity;    ///< Пропускная способность (в МВА)
    bool is_connected;  ///< Флаг включенного состояния
    
    /**
     * @brief Конструктор структуры ветви
     * @param id Идентификатор ветви
     * @param from Идентификатор начального узла
     * @param to Идентификатор конечного узла
     * @param r Активное сопротивление
     * @param x Реактивное сопротивление
     * @param cap Пропускная способность в МВА
     * @param connected Флаг включенного состояния
     */
    Branch(BranchId id = -1, NodeId from = -1, NodeId to = -1, 
           double r = 0.0, double x = 0.0, double cap = 0.0, bool connected = true)
        : id(id), from_node(from), to_node(to), resistance(r), 
          reactance(x), capacity(cap), is_connected(connected) {}
};

/**
 * @struct Generator
 * @brief Структура для представления генератора
 */
struct Generator {
    GeneratorId id;    ///< Идентификатор генератора
    NodeId node_id;    ///< Идентификатор узла, к которому подключен генератор
    std::string name;  ///< Наименование генератора
    double power;      ///< Активная мощность (в МВт)
    double angle;      ///< Угол ротора (в градусах)
    double frequency;  ///< Частота (в Гц)
    double inertia;    ///< Постоянная инерции (в секундах)
    
    /**
     * @brief Конструктор структуры генератора
     * @param id Идентификатор генератора
     * @param node Идентификатор узла
     * @param name Наименование генератора
     * @param power Активная мощность в МВт
     * @param angle Угол ротора в градусах
     * @param freq Частота в Гц
     * @param inertia Постоянная инерции в секундах
     */
    Generator(GeneratorId id = -1, NodeId node = -1, const std::string& name = "",
              double power = 0.0, double angle = 0.0, double freq = 50.0, double inertia = 0.0)
        : id(id), node_id(node), name(name), power(power), 
          angle(angle), frequency(freq), inertia(inertia) {}
};

/**
 * @struct Load
 * @brief Структура для представления нагрузки
 */
struct Load {
    LoadId id;             ///< Идентификатор нагрузки
    NodeId node_id;        ///< Идентификатор узла, к которому подключена нагрузка
    double active_power;   ///< Активная мощность (в МВт)
    double reactive_power; ///< Реактивная мощность (в МВАр)
    
    /**
     * @brief Конструктор структуры нагрузки
     * @param id Идентификатор нагрузки
     * @param node Идентификатор узла
     * @param p Активная мощность в МВт
     * @param q Реактивная мощность в МВАр
     */
    Load(LoadId id = -1, NodeId node = -1, double p = 0.0, double q = 0.0)
        : id(id), node_id(node), active_power(p), reactive_power(q) {}
};

/**
 * @struct Subsystem
 * @brief Структура для представления колебательной подсистемы
 */
struct Subsystem {
    SubsystemId id;                  ///< Идентификатор подсистемы
    std::vector<GeneratorId> generators; ///< Список генераторов в подсистеме
    double avg_angle;                ///< Средний угол роторов в подсистеме (в градусах)
    double avg_frequency;            ///< Средняя частота в подсистеме (в Гц)
    double kinetic_energy;           ///< Кинетическая энергия подсистемы (в Дж)
    
    /**
     * @brief Конструктор структуры подсистемы
     * @param id Идентификатор подсистемы
     */
    Subsystem(SubsystemId id = -1) : id(id), avg_angle(0.0), 
                                      avg_frequency(0.0), kinetic_energy(0.0) {}
};

/**
 * @enum ControlActionType
 * @brief Перечисление типов управляющих воздействий
 */
enum class ControlActionType {
    GENERATOR_TRIP,      ///< Отключение генератора
    LOAD_SHEDDING,       ///< Отключение нагрузки
    LINE_TRIP,           ///< Отключение линии
    POWER_REDUCTION,     ///< Снижение мощности генератора
    EXCITATION_CONTROL   ///< Управление возбуждением
};

/**
 * @struct ControlAction
 * @brief Структура для представления управляющего воздействия
 */
struct ControlAction {
    ControlActionType type;   ///< Тип управляющего воздействия
    int target_id;            ///< ID объекта воздействия
    double value;             ///< Значение воздействия (если применимо)
    int priority;             ///< Приоритет воздействия
    double estimated_effect;  ///< Ожидаемый эффект
    
    /**
     * @brief Конструктор структуры управляющего воздействия
     * @param type Тип воздействия
     * @param target Идентификатор целевого объекта
     * @param val Значение воздействия
     * @param prio Приоритет воздействия
     * @param effect Ожидаемый эффект
     */
    ControlAction(ControlActionType type = ControlActionType::GENERATOR_TRIP,
                  int target = -1, double val = 0.0, int prio = 0, double effect = 0.0)
        : type(type), target_id(target), value(val), 
          priority(prio), estimated_effect(effect) {}
};

/**
 * @struct PredictionResult
 * @brief Структура для представления результата прогнозирования
 */
struct PredictionResult {
    bool is_stable;                          ///< Флаг устойчивости системы
    double time_to_instability;              ///< Время до нарушения устойчивости (в секундах)
    std::vector<Subsystem> subsystems;       ///< Список подсистем в прогнозе
    std::vector<ControlAction> suggested_actions; ///< Рекомендуемые управляющие воздействия
    
    /**
     * @brief Конструктор результата прогнозирования
     */
    PredictionResult() : is_stable(true), time_to_instability(-1.0) {}
};

/**
 * @enum Status
 * @brief Перечисление статусов выполнения операций
 */
enum class Status {
    OK,        ///< Операция выполнена успешно
    WARNING,   ///< Операция выполнена с предупреждениями
    ERROR,     ///< Ошибка при выполнении операции
    CRITICAL   ///< Критическая ошибка
};

} // namespace common
} // namespace rtems 