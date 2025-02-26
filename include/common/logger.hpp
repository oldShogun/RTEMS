#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <mutex>
#include <memory>
#include <vector>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace rtems {
namespace common {

// Уровни логирования
enum class LogLevel {
    TRACE,  // Очень подробная информация для отладки
    DEBUG,  // Подробная информация для отладки
    INFO,   // Общая информация о работе программы
    WARNING,// Предупреждения, не критичные для работы программы
    ERROR,  // Ошибки, которые могут влиять на работу программы
    FATAL   // Критические ошибки, приводящие к завершению программы
};

// Приемник логов
class LogSink {
public:
    virtual ~LogSink() = default;
    virtual void write(LogLevel level, const std::string& message) = 0;
};

// Приемник логов для консоли
class ConsoleSink : public LogSink {
public:
    void write(LogLevel level, const std::string& message) override;
};

// Приемник логов для файла
class FileSink : public LogSink {
public:
    explicit FileSink(const std::string& filename);
    ~FileSink() override;
    void write(LogLevel level, const std::string& message) override;

private:
    std::ofstream file_;
};

// Основной класс логгера
class Logger {
public:
    // Получение глобального экземпляра логгера (Singleton)
    static Logger& instance();

    // Удаление конструкторов копирования/перемещения и операторов присваивания
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    // Установка уровня логирования
    void set_level(LogLevel level);
    
    // Получение текущего уровня логирования
    LogLevel get_level() const;

    // Добавление приемника логов
    void add_sink(std::shared_ptr<LogSink> sink);
    
    // Очистка всех приемников
    void clear_sinks();

    // Методы для логирования различных уровней
    void trace(const std::string& message);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void fatal(const std::string& message);

    // Общий метод для логирования
    void log(LogLevel level, const std::string& message);

private:
    // Приватный конструктор (для Singleton)
    Logger();

    // Уровень логирования
    LogLevel level_;

    // Список приемников логов
    std::vector<std::shared_ptr<LogSink>> sinks_;

    // Мьютекс для потокобезопасности
    mutable std::mutex mutex_;

    // Получение строкового представления уровня логирования
    std::string level_to_string(LogLevel level) const;

    // Получение текущей даты и времени в формате строки
    std::string get_current_timestamp() const;
};

// Вспомогательные макросы для удобного логирования
#define LOG_TRACE(message) rtems::common::Logger::instance().trace(message)
#define LOG_DEBUG(message) rtems::common::Logger::instance().debug(message)
#define LOG_INFO(message) rtems::common::Logger::instance().info(message)
#define LOG_WARNING(message) rtems::common::Logger::instance().warning(message)
#define LOG_ERROR(message) rtems::common::Logger::instance().error(message)
#define LOG_FATAL(message) rtems::common::Logger::instance().fatal(message)

} // namespace common
} // namespace rtems 