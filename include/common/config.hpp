#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <fstream>
#include "types.hpp"
#include "exceptions.hpp"

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

/**
 * @class Config
 * @brief Класс для управления конфигурацией системы
 *
 * Предоставляет функционал для загрузки, хранения и доступа к
 * параметрам конфигурации различных типов.
 */
class Config {
public:
    /**
     * @brief Конструктор по умолчанию
     *
     * Создает пустую конфигурацию без параметров.
     */
    Config() = default;
    
    /**
     * @brief Конструктор с загрузкой из файла
     * @param config_file Путь к файлу конфигурации
     * @throw ConfigError если возникла ошибка при чтении файла
     */
    explicit Config(const std::string& config_file);

    /**
     * @brief Загрузка конфигурации из файла
     * @param config_file Путь к файлу конфигурации
     * @throw ConfigError если возникла ошибка при чтении файла
     */
    void load_from_file(const std::string& config_file);
    
    /**
     * @brief Установка строкового значения
     * @param key Ключ параметра
     * @param value Строковое значение
     */
    void set_string(const std::string& key, const std::string& value);
    
    /**
     * @brief Установка целочисленного значения
     * @param key Ключ параметра
     * @param value Целочисленное значение
     */
    void set_int(const std::string& key, int value);
    
    /**
     * @brief Установка значения с плавающей точкой
     * @param key Ключ параметра
     * @param value Значение с плавающей точкой
     */
    void set_double(const std::string& key, double value);
    
    /**
     * @brief Установка логического значения
     * @param key Ключ параметра
     * @param value Логическое значение (true/false)
     */
    void set_bool(const std::string& key, bool value);
    
    /**
     * @brief Получение строкового значения
     * @param key Ключ параметра
     * @param default_value Значение по умолчанию, возвращаемое если ключ не найден
     * @return Строковое значение параметра или значение по умолчанию
     */
    std::string get_string(const std::string& key, const std::string& default_value = "") const;
    
    /**
     * @brief Получение целочисленного значения
     * @param key Ключ параметра
     * @param default_value Значение по умолчанию, возвращаемое если ключ не найден
     * @return Целочисленное значение параметра или значение по умолчанию
     */
    int get_int(const std::string& key, int default_value = 0) const;
    
    /**
     * @brief Получение значения с плавающей точкой
     * @param key Ключ параметра
     * @param default_value Значение по умолчанию, возвращаемое если ключ не найден
     * @return Значение с плавающей точкой или значение по умолчанию
     */
    double get_double(const std::string& key, double default_value = 0.0) const;
    
    /**
     * @brief Получение логического значения
     * @param key Ключ параметра
     * @param default_value Значение по умолчанию, возвращаемое если ключ не найден
     * @return Логическое значение параметра или значение по умолчанию
     */
    bool get_bool(const std::string& key, bool default_value = false) const;
    
    /**
     * @brief Проверка наличия ключа в конфигурации
     * @param key Ключ для проверки
     * @return true если ключ существует, false в противном случае
     */
    bool has_key(const std::string& key) const;
    
    /**
     * @brief Получение списка всех ключей в конфигурации
     * @return Вектор строк с ключами
     */
    std::vector<std::string> get_keys() const;
    
    /**
     * @brief Создание конфигурации по умолчанию с предустановленными значениями
     * @return Объект конфигурации с значениями по умолчанию
     */
    static Config create_default();
    
private:
    std::map<std::string, std::string> config_values_; ///< Хранилище пар ключ-значение
};

} // namespace common
} // namespace rtems 