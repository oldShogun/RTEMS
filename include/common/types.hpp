#pragma once

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <cmath>
#include <stdexcept>

namespace rtems {
namespace common {

// Базовые константы
constexpr double PI = 3.14159265358979323846;
constexpr double DEFAULT_ANGLE_THRESHOLD = 20.0; // градусы
constexpr int MAX_PMU_FREQUENCY = 50; // измерений в секунду

// Идентификаторы
using NodeId = int;
using BranchId = int;
using GeneratorId = int;
using LoadId = int;
using SubsystemId = int;

// Структура узла сети
struct Node {
    NodeId id;
    std::string name;
    double voltage;
    double angle;
    bool is_generator;
    
    Node(NodeId id = -1, const std::string& name = "", double voltage = 0.0, 
         double angle = 0.0, bool is_generator = false)
        : id(id), name(name), voltage(voltage), angle(angle), is_generator(is_generator) {}
};

// Структура ветви сети
struct Branch {
    BranchId id;
    NodeId from_node;
    NodeId to_node;
    double resistance;
    double reactance;
    double capacity;
    bool is_connected;
    
    Branch(BranchId id = -1, NodeId from = -1, NodeId to = -1, 
           double r = 0.0, double x = 0.0, double cap = 0.0, bool connected = true)
        : id(id), from_node(from), to_node(to), resistance(r), 
          reactance(x), capacity(cap), is_connected(connected) {}
};

// Структура генератора
struct Generator {
    GeneratorId id;
    NodeId node_id;
    std::string name;
    double power;
    double angle;
    double frequency;
    double inertia;
    
    Generator(GeneratorId id = -1, NodeId node = -1, const std::string& name = "",
              double power = 0.0, double angle = 0.0, double freq = 50.0, double inertia = 0.0)
        : id(id), node_id(node), name(name), power(power), 
          angle(angle), frequency(freq), inertia(inertia) {}
};

// Структура нагрузки
struct Load {
    LoadId id;
    NodeId node_id;
    double active_power;
    double reactive_power;
    
    Load(LoadId id = -1, NodeId node = -1, double p = 0.0, double q = 0.0)
        : id(id), node_id(node), active_power(p), reactive_power(q) {}
};

// Структура колебательной подсистемы
struct Subsystem {
    SubsystemId id;
    std::vector<GeneratorId> generators;
    double avg_angle;
    double avg_frequency;
    double kinetic_energy;
    
    Subsystem(SubsystemId id = -1) : id(id), avg_angle(0.0), 
                                      avg_frequency(0.0), kinetic_energy(0.0) {}
};

// Типы управляющих воздействий
enum class ControlActionType {
    GENERATOR_TRIP,      // Отключение генератора
    LOAD_SHEDDING,       // Отключение нагрузки
    LINE_TRIP,           // Отключение линии
    POWER_REDUCTION,     // Снижение мощности генератора
    EXCITATION_CONTROL   // Управление возбуждением
};

// Структура управляющего воздействия
struct ControlAction {
    ControlActionType type;
    int target_id;       // ID объекта воздействия
    double value;        // Значение воздействия (если применимо)
    int priority;        // Приоритет воздействия
    double estimated_effect; // Ожидаемый эффект
    
    ControlAction(ControlActionType type = ControlActionType::GENERATOR_TRIP,
                  int target = -1, double val = 0.0, int prio = 0, double effect = 0.0)
        : type(type), target_id(target), value(val), 
          priority(prio), estimated_effect(effect) {}
};

// Результат прогнозирования
struct PredictionResult {
    bool is_stable;
    double time_to_instability;
    std::vector<Subsystem> subsystems;
    std::vector<ControlAction> suggested_actions;
    
    PredictionResult() : is_stable(true), time_to_instability(-1.0) {}
};

// Статус выполнения
enum class Status {
    OK,
    WARNING,
    ERROR,
    CRITICAL
};

} // namespace common
} // namespace rtems 