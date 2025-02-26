#include "../../include/common/logger.hpp"

namespace rtems {
namespace common {

// Реализация методов ConsoleSink
void ConsoleSink::write(LogLevel level, const std::string& message) {
    // В зависимости от уровня логирования выводим в stdout или stderr
    if (level >= LogLevel::ERROR) {
        std::cerr << message << std::endl;
    } else {
        std::cout << message << std::endl;
    }
}

// Реализация методов FileSink
FileSink::FileSink(const std::string& filename) {
    file_.open(filename, std::ios::out | std::ios::app);
    if (!file_.is_open()) {
        throw std::runtime_error("Failed to open log file: " + filename);
    }
}

FileSink::~FileSink() {
    if (file_.is_open()) {
        file_.close();
    }
}

void FileSink::write(LogLevel level, const std::string& message) {
    if (file_.is_open()) {
        file_ << message << std::endl;
    }
}

// Реализация методов Logger
Logger::Logger() : level_(LogLevel::INFO) {
    // По умолчанию добавляем приемник для консоли
    add_sink(std::make_shared<ConsoleSink>());
}

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

void Logger::set_level(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
}

LogLevel Logger::get_level() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return level_;
}

void Logger::add_sink(std::shared_ptr<LogSink> sink) {
    std::lock_guard<std::mutex> lock(mutex_);
    sinks_.push_back(sink);
}

void Logger::clear_sinks() {
    std::lock_guard<std::mutex> lock(mutex_);
    sinks_.clear();
}

void Logger::trace(const std::string& message) {
    log(LogLevel::TRACE, message);
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::fatal(const std::string& message) {
    log(LogLevel::FATAL, message);
}

void Logger::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Проверяем, нужно ли логировать данное сообщение
    if (level < level_) {
        return;
    }
    
    // Формируем полное сообщение с отметкой времени и уровнем
    std::string formatted_message = 
        get_current_timestamp() + " [" + level_to_string(level) + "] " + message;
    
    // Отправляем сообщение во все приемники
    for (auto& sink : sinks_) {
        sink->write(level, formatted_message);
    }
}

std::string Logger::level_to_string(LogLevel level) const {
    switch (level) {
        case LogLevel::TRACE:   return "TRACE";
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR:   return "ERROR";
        case LogLevel::FATAL:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

std::string Logger::get_current_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    
    // Получаем миллисекунды
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}

} // namespace common
} // namespace rtems 