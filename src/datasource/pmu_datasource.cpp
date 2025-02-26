#include "../../include/datasource/pmu_datasource.hpp"
#include "../../include/common/exceptions.hpp"
#include <algorithm>
#include <cmath>
#include <random>

namespace rtems {
namespace datasource {

PMUDataSource::PMUDataSource(const common::Config& config)
    : config_(config) {
}

PMUDataSource::~PMUDataSource() {
    // Закрываем соединение, если оно было открыто
    if (connected_) {
        // В реальной реализации здесь будет код для закрытия соединения с PMU
        connected_ = false;
    }
}

void PMUDataSource::initialize() {
    if (initialized_) {
        return;
    }
    
    // В MVP мы загружаем тестовые данные вместо реального подключения к PMU
    load_test_data();
    
    // Создаем карты для быстрого доступа
    for (const auto& node : nodes_) {
        node_map_[node.id] = node;
    }
    
    for (const auto& generator : generators_) {
        generator_map_[generator.id] = generator;
    }
    
    // Устанавливаем флаги
    initialized_ = true;
    connected_ = true;
}

bool PMUDataSource::is_connected() const {
    return connected_;
}

std::string PMUDataSource::get_name() const {
    return name_;
}

std::vector<common::Node> PMUDataSource::get_nodes() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return nodes_;
}

std::vector<common::Branch> PMUDataSource::get_branches() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return branches_;
}

std::vector<common::Generator> PMUDataSource::get_generators() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return generators_;
}

std::vector<common::Load> PMUDataSource::get_loads() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return loads_;
}

void PMUDataSource::update() {
    if (!connected_) {
        throw common::DataSourceException("PMU source is not connected");
    }
    
    // В MVP мы просто добавляем случайные колебания к углам и частотам
    std::lock_guard<std::mutex> lock(data_mutex_);
    
    // Генератор случайных чисел
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::normal_distribution<> angle_noise(0.0, 0.1); // небольшие колебания углов
    static std::normal_distribution<> freq_noise(0.0, 0.01); // небольшие колебания частоты
    
    // Обновляем данные генераторов
    for (auto& generator : generators_) {
        // Добавляем небольшие случайные изменения к углам и частотам
        generator.angle += angle_noise(gen);
        generator.frequency = 50.0 + freq_noise(gen);
        
        // Обновляем карту
        generator_map_[generator.id] = generator;
    }
    
    // Обновляем данные узлов
    for (auto& node : nodes_) {
        if (node.is_generator) {
            // Для узлов с генераторами угол берется из соответствующего генератора
            for (const auto& generator : generators_) {
                if (generator.node_id == node.id) {
                    node.angle = generator.angle;
                    break;
                }
            }
        } else {
            // Для обычных узлов добавляем небольшие колебания
            node.angle += angle_noise(gen);
        }
        
        // Обновляем карту
        node_map_[node.id] = node;
    }
}

double PMUDataSource::get_generator_angle(common::GeneratorId generator_id) const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = generator_map_.find(generator_id);
    if (it == generator_map_.end()) {
        throw common::GeneratorNotFoundException(generator_id);
    }
    return it->second.angle;
}

double PMUDataSource::get_generator_frequency(common::GeneratorId generator_id) const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = generator_map_.find(generator_id);
    if (it == generator_map_.end()) {
        throw common::GeneratorNotFoundException(generator_id);
    }
    return it->second.frequency;
}

double PMUDataSource::get_generator_power(common::GeneratorId generator_id) const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = generator_map_.find(generator_id);
    if (it == generator_map_.end()) {
        throw common::GeneratorNotFoundException(generator_id);
    }
    return it->second.power;
}

double PMUDataSource::get_node_voltage(common::NodeId node_id) const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = node_map_.find(node_id);
    if (it == node_map_.end()) {
        throw common::NodeNotFoundException(node_id);
    }
    return it->second.voltage;
}

double PMUDataSource::get_node_angle(common::NodeId node_id) const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    auto it = node_map_.find(node_id);
    if (it == node_map_.end()) {
        throw common::NodeNotFoundException(node_id);
    }
    return it->second.angle;
}

void PMUDataSource::load_test_data() {
    // Создаем тестовую систему с 6 узлами, 7 ветвями и 3 генераторами
    
    // Узлы
    nodes_ = {
        common::Node(1, "Node 1", 110.0, 0.0, true),
        common::Node(2, "Node 2", 110.0, -5.0, false),
        common::Node(3, "Node 3", 110.0, -8.0, false),
        common::Node(4, "Node 4", 110.0, -10.0, true),
        common::Node(5, "Node 5", 110.0, -12.0, false),
        common::Node(6, "Node 6", 110.0, -15.0, true)
    };
    
    // Ветви
    branches_ = {
        common::Branch(1, 1, 2, 0.02, 0.1, 100.0),
        common::Branch(2, 2, 3, 0.02, 0.1, 100.0),
        common::Branch(3, 3, 4, 0.02, 0.1, 100.0),
        common::Branch(4, 2, 5, 0.02, 0.1, 100.0),
        common::Branch(5, 3, 5, 0.02, 0.1, 100.0),
        common::Branch(6, 4, 5, 0.02, 0.1, 100.0),
        common::Branch(7, 5, 6, 0.02, 0.1, 100.0)
    };
    
    // Генераторы
    generators_ = {
        common::Generator(1, 1, "Generator 1", 100.0, 0.0, 50.0, 5.0),
        common::Generator(2, 4, "Generator 2", 80.0, -10.0, 50.0, 4.0),
        common::Generator(3, 6, "Generator 3", 120.0, -15.0, 50.0, 6.0)
    };
    
    // Нагрузки
    loads_ = {
        common::Load(1, 2, 50.0, 15.0),
        common::Load(2, 3, 60.0, 20.0),
        common::Load(3, 5, 70.0, 25.0)
    };
}

} // namespace datasource
} // namespace rtems 