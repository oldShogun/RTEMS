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
 * @enum LogLevel
 * @brief Уровни логирования, определяющие важность сообщения
 */
enum class LogLevel {
    TRACE,  ///< Очень подробная информация для отладки
    DEBUG,  ///< Подробная информация для отладки
    INFO,   ///< Общая информация о работе программы
    WARNING,///< Предупреждения, не критичные для работы программы
    ERROR,  ///< Ошибки, которые могут влиять на работу программы
    FATAL   ///< Критические ошибки, приводящие к завершению программы
};

/**
 * @class LogSink
 * @brief Абстрактный базовый класс для всех приемников логов
 *
 * Определяет интерфейс для вывода сообщений логирования в различные назначения
 */
class LogSink {
public:
    /**
     * @brief Виртуальный деструктор по умолчанию
     */
    virtual ~LogSink() = default;
    
    /**
     * @brief Запись сообщения с указанным уровнем важности
     * @param level Уровень важности сообщения
     * @param message Текст сообщения
     */
    virtual void write(LogLevel level, const std::string& message) = 0;
};

/**
 * @class ConsoleSink
 * @brief Приемник логов для вывода в консоль
 *
 * Выводит сообщения в стандартный вывод (stdout) или стандартный поток ошибок (stderr)
 * в зависимости от уровня логирования
 */
class ConsoleSink : public LogSink {
public:
    /**
     * @brief Запись сообщения в консоль
     * @param level Уровень важности сообщения
     * @param message Текст сообщения
     */
    void write(LogLevel level, const std::string& message) override;
};

/**
 * @class FileSink
 * @brief Приемник логов для записи в файл
 *
 * Записывает все сообщения в указанный файл логирования
 */
class FileSink : public LogSink {
public:
    /**
     * @brief Конструктор, открывающий файл для логирования
     * @param filename Имя файла для записи логов
     * @throw std::runtime_error если не удалось открыть файл
     */
    explicit FileSink(const std::string& filename);
    
    /**
     * @brief Деструктор, закрывающий файл логирования
     */
    ~FileSink() override;
    
    /**
     * @brief Запись сообщения в файл
     * @param level Уровень важности сообщения
     * @param message Текст сообщения
     */
    void write(LogLevel level, const std::string& message) override;

private:
    std::ofstream file_; ///< Файловый поток для записи
};

/**
 * @class Logger
 * @brief Основной класс для логирования сообщений
 *
 * Реализует паттерн Singleton для централизованного логирования.
 * Поддерживает несколько приемников логов и фильтрацию по уровню важности.
 */
class Logger {
public:
    /**
     * @brief Получение глобального экземпляра логгера (Singleton)
     * @return Ссылка на единственный экземпляр Logger
     */
    static Logger& instance();

    // Удаление конструкторов копирования/перемещения и операторов присваивания
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    /**
     * @brief Установка минимального уровня логирования
     * @param level Новый уровень логирования
     */
    void set_level(LogLevel level);
    
    /**
     * @brief Получение текущего уровня логирования
     * @return Текущий уровень логирования
     */
    LogLevel get_level() const;

    /**
     * @brief Добавление приемника логов
     * @param sink Умный указатель на приемник логов
     */
    void add_sink(std::shared_ptr<LogSink> sink);
    
    /**
     * @brief Очистка всех приемников логов
     */
    void clear_sinks();

    /**
     * @brief Логирование сообщения с уровнем TRACE
     * @param message Текст сообщения
     */
    void trace(const std::string& message);
    
    /**
     * @brief Логирование сообщения с уровнем DEBUG
     * @param message Текст сообщения
     */
    void debug(const std::string& message);
    
    /**
     * @brief Логирование сообщения с уровнем INFO
     * @param message Текст сообщения
     */
    void info(const std::string& message);
    
    /**
     * @brief Логирование сообщения с уровнем WARNING
     * @param message Текст сообщения
     */
    void warning(const std::string& message);
    
    /**
     * @brief Логирование сообщения с уровнем ERROR
     * @param message Текст сообщения
     */
    void error(const std::string& message);
    
    /**
     * @brief Логирование сообщения с уровнем FATAL
     * @param message Текст сообщения
     */
    void fatal(const std::string& message);

    /**
     * @brief Общий метод для логирования с произвольным уровнем
     * @param level Уровень важности сообщения
     * @param message Текст сообщения
     */
    void log(LogLevel level, const std::string& message);

private:
    /**
     * @brief Приватный конструктор (для Singleton)
     */
    Logger();

    LogLevel level_; ///< Текущий уровень логирования
    std::vector<std::shared_ptr<LogSink>> sinks_; ///< Список приемников логов
    mutable std::mutex mutex_; ///< Мьютекс для потокобезопасности

    /**
     * @brief Получение строкового представления уровня логирования
     * @param level Уровень логирования
     * @return Строковое представление уровня
     */
    std::string level_to_string(LogLevel level) const;

    /**
     * @brief Получение текущей даты и времени в формате строки
     * @return Строка с текущей датой и временем
     */
    std::string get_current_timestamp() const;
};

/**
 * @def LOG_TRACE(message)
 * @brief Макрос для логирования сообщения с уровнем TRACE
 * @param message Текст сообщения
 */
#define LOG_TRACE(message) rtems::common::Logger::instance().trace(message)

/**
 * @def LOG_DEBUG(message)
 * @brief Макрос для логирования сообщения с уровнем DEBUG
 * @param message Текст сообщения
 */
#define LOG_DEBUG(message) rtems::common::Logger::instance().debug(message)

/**
 * @def LOG_INFO(message)
 * @brief Макрос для логирования сообщения с уровнем INFO
 * @param message Текст сообщения
 */
#define LOG_INFO(message) rtems::common::Logger::instance().info(message)

/**
 * @def LOG_WARNING(message)
 * @brief Макрос для логирования сообщения с уровнем WARNING
 * @param message Текст сообщения
 */
#define LOG_WARNING(message) rtems::common::Logger::instance().warning(message)

/**
 * @def LOG_ERROR(message)
 * @brief Макрос для логирования сообщения с уровнем ERROR
 * @param message Текст сообщения
 */
#define LOG_ERROR(message) rtems::common::Logger::instance().error(message)

/**
 * @def LOG_FATAL(message)
 * @brief Макрос для логирования сообщения с уровнем FATAL
 * @param message Текст сообщения
 */
#define LOG_FATAL(message) rtems::common::Logger::instance().fatal(message)

} // namespace common
} // namespace rtems 