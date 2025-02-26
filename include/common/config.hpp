#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <fstream>
#include "types.hpp"
#include "exceptions.hpp"

namespace rtems {
namespace common {

// Простой класс конфигурации системы
class Config {
public:
    // Конструкторы
    Config() = default;
    explicit Config(const std::string& config_file);

    // Загрузка из файла
    void load_from_file(const std::string& config_file);
    
    // Установка значений
    void set_string(const std::string& key, const std::string& value);
    void set_int(const std::string& key, int value);
    void set_double(const std::string& key, double value);
    void set_bool(const std::string& key, bool value);
    
    // Получение значений с проверкой наличия
    std::string get_string(const std::string& key, const std::string& default_value = "") const;
    int get_int(const std::string& key, int default_value = 0) const;
    double get_double(const std::string& key, double default_value = 0.0) const;
    bool get_bool(const std::string& key, bool default_value = false) const;
    
    // Проверка наличия ключа
    bool has_key(const std::string& key) const;
    
    // Получение всех ключей
    std::vector<std::string> get_keys() const;
    
    // Создание конфигурации по умолчанию
    static Config create_default();
    
private:
    std::map<std::string, std::string> config_values_;
};

} // namespace common
} // namespace rtems 