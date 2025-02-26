#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <vector>
#include <memory>
#include <locale>
#include <codecvt>
#include <sstream>

#include "../include/common/config.hpp"
#include "../include/common/exceptions.hpp"
#include "../include/common/logger.hpp"
#include "../include/datasource/pmu_datasource.hpp"
#include "../include/monitoring/subsystem_detector.hpp"
#include "../include/prediction/fast_predictor.hpp"

using namespace rtems;
using namespace std::chrono_literals;

// Функция для вывода информации о подсистемах
void print_subsystems(const std::vector<common::Subsystem>& subsystems) {
    std::stringstream ss;
    ss << "Subsystems (" << subsystems.size() << "):";
    LOG_INFO(ss.str());
    
    for (const auto& subsystem : subsystems) {
        ss.str("");
        ss << "  Subsystem " << subsystem.id << ":";
        LOG_INFO(ss.str());
        
        ss.str("");
        ss << "    Generators: ";
        for (auto gen_id : subsystem.generators) {
            ss << gen_id << " ";
        }
        LOG_INFO(ss.str());
        
        ss.str("");
        ss << "    Average angle: " << std::fixed << std::setprecision(2) 
           << subsystem.avg_angle << " degrees";
        LOG_INFO(ss.str());
        
        ss.str("");
        ss << "    Average frequency: " << std::fixed << std::setprecision(3) 
           << subsystem.avg_frequency << " Hz";
        LOG_INFO(ss.str());
        
        ss.str("");
        ss << "    Kinetic energy: " << std::fixed << std::setprecision(1) 
           << subsystem.kinetic_energy << " J";
        LOG_INFO(ss.str());
    }
}

// Функция для вывода информации о результатах прогнозирования
void print_prediction_result(const common::PredictionResult& result) {
    LOG_INFO("Prediction result:");
    
    std::stringstream ss;
    ss << "  Stability: " << (result.is_stable ? "Stable" : "Unstable");
    LOG_INFO(ss.str());
    
    if (!result.is_stable) {
        ss.str("");
        ss << "  Time to instability: " << std::fixed << std::setprecision(2) 
           << result.time_to_instability << " s";
        LOG_INFO(ss.str());
    }
    
    LOG_INFO("  Predicted subsystems:");
    print_subsystems(result.subsystems);
    
    if (!result.suggested_actions.empty()) {
        LOG_INFO("  Recommended control actions:");
        
        for (const auto& action : result.suggested_actions) {
            ss.str("");
            ss << "    ";
            
            switch (action.type) {
                case common::ControlActionType::GENERATOR_TRIP:
                    ss << "Generator trip #" << action.target_id;
                    break;
                case common::ControlActionType::LOAD_SHEDDING:
                    ss << "Load shedding #" << action.target_id;
                    break;
                case common::ControlActionType::LINE_TRIP:
                    ss << "Line trip #" << action.target_id;
                    break;
                case common::ControlActionType::POWER_REDUCTION:
                    ss << "Generator power reduction #" << action.target_id 
                       << " by " << action.value << "%";
                    break;
                case common::ControlActionType::EXCITATION_CONTROL:
                    ss << "Generator excitation change #" << action.target_id 
                       << " by " << action.value << "%";
                    break;
                default:
                    ss << "Unknown action";
                    break;
            }
            
            ss << " (priority: " << action.priority 
               << ", expected effect: " << action.estimated_effect << ")";
            LOG_INFO(ss.str());
        }
    }
}

// Функция настройки логгера
void setup_logger(common::LogLevel level, const std::string& log_file = "") {
    auto& logger = common::Logger::instance();
    logger.set_level(level);
    
    // Очистка всех приемников (включая консольный по умолчанию)
    logger.clear_sinks();
    
    // Добавляем консольный приемник
    logger.add_sink(std::make_shared<common::ConsoleSink>());
    
    // Если указан файл лога, добавляем файловый приемник
    if (!log_file.empty()) {
        try {
            logger.add_sink(std::make_shared<common::FileSink>(log_file));
            LOG_INFO("Logging to file: " + log_file);
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to create file logger: " + std::string(e.what()));
        }
    }
}

int main() {
    try {
        // Настройка локали и кодировки для корректного отображения русских символов
        std::locale::global(std::locale(""));
        std::wcout.imbue(std::locale(""));
        std::cout.imbue(std::locale(""));
        
        // Настройка логгера
        setup_logger(common::LogLevel::DEBUG, "rtems_log.txt");
        
        // Выводим заголовок
        LOG_INFO("=====================================================");
        LOG_INFO("  RTEMS - Real-Time Electromechanical System Modeler");
        LOG_INFO("                     MVP Demo                        ");
        LOG_INFO("=====================================================");
        
        // Создаем конфигурацию по умолчанию
        common::Config config = common::Config::create_default();
        
        // Создаем источник данных PMU
        LOG_DEBUG("Creating PMU data source...");
        auto pmu_source = std::make_shared<datasource::PMUDataSource>(config);
        pmu_source->initialize();
        LOG_INFO("PMU data source initialized");
        
        // Создаем детектор подсистем
        LOG_DEBUG("Creating subsystem detector...");
        auto subsystem_detector = std::make_shared<monitoring::SubsystemDetector>(config);
        subsystem_detector->initialize(pmu_source);
        LOG_INFO("Subsystem detector initialized");
        
        // Создаем быстрый предиктор
        LOG_DEBUG("Creating fast predictor...");
        auto fast_predictor = std::make_shared<prediction::FastPredictor>(config);
        fast_predictor->initialize(subsystem_detector);
        LOG_INFO("Fast predictor initialized");
        
        // Горизонт прогнозирования (в секундах)
        double time_horizon = config.get_double("prediction.time_horizon", 5.0);
        
        // Основной цикл программы
        int iteration = 0;
        while (iteration++ < 10) { // Для демонстрации выполняем 10 итераций
            std::stringstream ss;
            ss << "----- Iteration " << iteration << " -----";
            LOG_INFO("\n" + ss.str());
            
            // Обновляем данные
            LOG_DEBUG("Updating data...");
            pmu_source->update();
            subsystem_detector->update();
            
            // Выводим информацию о подсистемах
            auto subsystems = subsystem_detector->get_subsystems();
            print_subsystems(subsystems);
            
            // Проверяем превышение порога по углу
            if (subsystem_detector->is_angle_threshold_exceeded()) {
                LOG_WARNING("Angle threshold exceeded between subsystems!");
                
                ss.str("");
                ss << "Maximum angle difference: " << std::fixed << std::setprecision(2) 
                   << subsystem_detector->get_max_angle_difference() << " degrees";
                LOG_WARNING(ss.str());
                
                // Получаем индексы критических подсистем
                auto [i1, i2] = subsystem_detector->get_critical_subsystem_indices();
                if (i1 >= 0 && i2 >= 0) {
                    ss.str("");
                    ss << "Critical subsystems: " << i1 + 1 << " and " << i2 + 1;
                    LOG_WARNING(ss.str());
                }
                
                // Запускаем прогнозирование
                ss.str("");
                ss << "Starting prediction with horizon " << time_horizon << " s...";
                LOG_INFO("\n" + ss.str());
                
                auto result = fast_predictor->predict(time_horizon);
                
                ss.str("");
                ss << "Prediction execution time: " << std::fixed << std::setprecision(1) 
                   << fast_predictor->get_last_execution_time() << " ms";
                LOG_DEBUG(ss.str());
                
                // Выводим результаты прогнозирования
                print_prediction_result(result);
            } else {
                ss.str("");
                ss << "System is stable. Maximum angle difference: " 
                   << std::fixed << std::setprecision(2) 
                   << subsystem_detector->get_max_angle_difference() 
                   << " degrees";
                LOG_INFO("\n" + ss.str());
            }
            
            // Ждем перед следующей итерацией
            LOG_TRACE("Waiting before next iteration...");
            std::this_thread::sleep_for(1s);
        }
        
        LOG_INFO("\nMVP demonstration completed");
        
    } catch (const common::RTEMSException& e) {
        LOG_FATAL("RTEMS Error: " + std::string(e.what()));
        return 1;
    } catch (const std::exception& e) {
        LOG_FATAL("General Error: " + std::string(e.what()));
        return 1;
    }
    
    return 0;
} 