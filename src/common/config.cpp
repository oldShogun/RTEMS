#include "../../include/common/config.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace rtems {
namespace common {

Config::Config(const std::string& config_file) {
    load_from_file(config_file);
}

void Config::load_from_file(const std::string& config_file) {
    std::ifstream file(config_file);
    if (!file.is_open()) {
        throw ConfigurationException("Could not open config file: " + config_file);
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Пропускаем комментарии и пустые строки
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        // Разбираем ключ=значение
        size_t delimiter_pos = line.find('=');
        if (delimiter_pos != std::string::npos) {
            std::string key = line.substr(0, delimiter_pos);
            std::string value = line.substr(delimiter_pos + 1);
            
            // Удаляем пробелы
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            config_values_[key] = value;
        }
    }
}

void Config::set_string(const std::string& key, const std::string& value) {
    config_values_[key] = value;
}

void Config::set_int(const std::string& key, int value) {
    config_values_[key] = std::to_string(value);
}

void Config::set_double(const std::string& key, double value) {
    config_values_[key] = std::to_string(value);
}

void Config::set_bool(const std::string& key, bool value) {
    config_values_[key] = value ? "true" : "false";
}

std::string Config::get_string(const std::string& key, const std::string& default_value) const {
    auto it = config_values_.find(key);
    return (it != config_values_.end()) ? it->second : default_value;
}

int Config::get_int(const std::string& key, int default_value) const {
    auto it = config_values_.find(key);
    if (it != config_values_.end()) {
        try {
            return std::stoi(it->second);
        } catch (const std::exception&) {
            // Если произошла ошибка преобразования, возвращаем значение по умолчанию
        }
    }
    return default_value;
}

double Config::get_double(const std::string& key, double default_value) const {
    auto it = config_values_.find(key);
    if (it != config_values_.end()) {
        try {
            return std::stod(it->second);
        } catch (const std::exception&) {
            // Если произошла ошибка преобразования, возвращаем значение по умолчанию
        }
    }
    return default_value;
}

bool Config::get_bool(const std::string& key, bool default_value) const {
    auto it = config_values_.find(key);
    if (it != config_values_.end()) {
        std::string value = it->second;
        // Преобразуем к нижнему регистру для сравнения
        std::transform(value.begin(), value.end(), value.begin(), 
                     [](unsigned char c){ return std::tolower(c); });
        
        if (value == "true" || value == "1" || value == "yes" || value == "y") {
            return true;
        } else if (value == "false" || value == "0" || value == "no" || value == "n") {
            return false;
        }
    }
    return default_value;
}

bool Config::has_key(const std::string& key) const {
    return config_values_.find(key) != config_values_.end();
}

std::vector<std::string> Config::get_keys() const {
    std::vector<std::string> keys;
    for (const auto& pair : config_values_) {
        keys.push_back(pair.first);
    }
    return keys;
}

Config Config::create_default() {
    Config config;
    
    // Основные параметры системы
    config.set_string("system.name", "RTEMS");
    config.set_string("system.version", "0.1.0");
    config.set_int("system.pmu_frequency", 50);
    
    // Параметры мониторинга
    config.set_double("monitoring.angle_threshold", 20.0);
    config.set_double("monitoring.frequency_threshold", 0.5);
    config.set_int("monitoring.window_size", 10);
    
    // Параметры прогнозирования
    config.set_double("prediction.time_horizon", 5.0);
    config.set_double("prediction.time_step", 0.01);
    config.set_bool("prediction.use_fast_predictor", true);
    
    // Параметры моделирования
    config.set_int("simulation.max_iterations", 100);
    config.set_double("simulation.convergence_threshold", 1e-6);
    
    // Параметры управляющих воздействий
    config.set_bool("control.enable_actions", true);
    config.set_int("control.max_actions", 5);
    
    return config;
}

} // namespace common
} // namespace rtems 