/**
 * @file circular_buffer.hpp
 * @brief Реализация кольцевого буфера для хранения временных рядов
 * @details Кольцевой буфер позволяет эффективно хранить фиксированный объем данных временного ряда
 *          с автоматическим вытеснением самых старых значений при добавлении новых
 * 
 * @author RTEMS Team
 * @date 2023
 */

#pragma once

#include <vector>
#include <mutex>
#include <stdexcept>
#include <algorithm>

#include "istorage.hpp"

namespace rtems {
namespace storage {

/**
 * @brief Кольцевой буфер для хранения временных рядов
 * 
 * @tparam T Тип данных, хранимых в буфере
 * @details Класс реализует кольцевой буфер, который автоматически вытесняет старые данные
 *          при добавлении новых, если размер буфера превышает заданную емкость.
 *          Буфер является потокобезопасным и может быть использован в многопоточной среде.
 */
template <typename T>
class CircularBuffer {
public:
    /**
     * @brief Конструктор буфера с указанием емкости
     * 
     * @param capacity Максимальная емкость буфера (количество элементов)
     */
    explicit CircularBuffer(std::size_t capacity)
        : m_capacity(capacity), m_size(0), m_head(0) {
        if (capacity == 0) {
            throw std::invalid_argument("CircularBuffer capacity must be greater than 0");
        }
        m_buffer.resize(capacity);
    }

    /**
     * @brief Добавить элемент в буфер
     * 
     * @param item Добавляемый элемент
     * @details Добавляет элемент в буфер. Если буфер полон, самый старый элемент вытесняется.
     */
    void push(const T& item) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        m_buffer[m_head] = item;
        
        if (m_size < m_capacity) {
            m_size++;
        }
        
        m_head = (m_head + 1) % m_capacity;
    }

    /**
     * @brief Добавить элемент в буфер, используя перемещение
     * 
     * @param item Добавляемый элемент
     * @details Добавляет элемент в буфер с использованием семантики перемещения. 
     *          Если буфер полон, самый старый элемент вытесняется.
     */
    void push(T&& item) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        m_buffer[m_head] = std::move(item);
        
        if (m_size < m_capacity) {
            m_size++;
        }
        
        m_head = (m_head + 1) % m_capacity;
    }

    /**
     * @brief Получить копию всех данных из буфера в порядке от старых к новым
     * 
     * @return std::vector<T> Вектор с копией данных
     */
    std::vector<T> getAll() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        std::vector<T> result;
        result.reserve(m_size);
        
        if (m_size == 0) {
            return result;
        }
        
        // Вычисляем индекс начала данных (самых старых)
        std::size_t start = m_size < m_capacity ? 0 : m_head;
        
        // Собираем данные от самых старых к самым новым
        for (std::size_t i = 0; i < m_size; ++i) {
            std::size_t index = (start + i) % m_capacity;
            result.push_back(m_buffer[index]);
        }
        
        return result;
    }

    /**
     * @brief Получить последние N элементов буфера
     * 
     * @param n Количество последних элементов для получения
     * @return std::vector<T> Вектор с последними n элементами
     * @details Если n больше размера буфера, возвращаются все элементы буфера
     */
    std::vector<T> getLast(std::size_t n) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (n == 0 || m_size == 0) {
            return std::vector<T>();
        }
        
        n = std::min(n, m_size);
        std::vector<T> result;
        result.reserve(n);
        
        // Вычисляем индекс начала последних n элементов
        std::size_t start = (m_head - n + m_capacity) % m_capacity;
        
        // Собираем последние n элементов
        for (std::size_t i = 0; i < n; ++i) {
            std::size_t index = (start + i) % m_capacity;
            result.push_back(m_buffer[index]);
        }
        
        return result;
    }

    /**
     * @brief Очистить буфер
     */
    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        m_size = 0;
        m_head = 0;
    }

    /**
     * @brief Получить текущий размер буфера
     * 
     * @return std::size_t Текущий размер буфера (количество элементов)
     */
    std::size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_size;
    }

    /**
     * @brief Получить емкость буфера
     * 
     * @return std::size_t Емкость буфера (максимальное количество элементов)
     */
    std::size_t capacity() const {
        return m_capacity;
    }

    /**
     * @brief Проверить, пуст ли буфер
     * 
     * @return true если буфер пуст
     * @return false если буфер содержит элементы
     */
    bool empty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_size == 0;
    }

    /**
     * @brief Проверить, полон ли буфер
     * 
     * @return true если буфер полон
     * @return false если в буфере есть свободное место
     */
    bool full() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_size == m_capacity;
    }

private:
    std::vector<T> m_buffer;          ///< Контейнер для хранения данных
    std::size_t m_capacity;           ///< Максимальная емкость буфера
    std::size_t m_size;               ///< Текущий размер буфера
    std::size_t m_head;               ///< Указатель на позицию добавления следующего элемента
    mutable std::mutex m_mutex;       ///< Мьютекс для обеспечения потокобезопасности
};

/**
 * @brief Специализация кольцевого буфера для хранения временных рядов
 * 
 * @details Класс предоставляет интерфейс для хранения пар "значение-временная метка"
 *          с дополнительными методами для работы с временным рядом
 */
class TimeSeriesCircularBuffer {
public:
    /**
     * @brief Конструктор буфера временного ряда с указанием емкости
     * 
     * @param capacity Максимальная емкость буфера (количество точек временного ряда)
     */
    explicit TimeSeriesCircularBuffer(std::size_t capacity)
        : m_valueBuffer(capacity), m_timeBuffer(capacity) {}

    /**
     * @brief Добавить точку временного ряда в буфер
     * 
     * @param value Значение
     * @param timestamp Временная метка
     */
    void push(const Value& value, const TimePoint& timestamp) {
        m_valueBuffer.push(value);
        m_timeBuffer.push(timestamp);
    }

    /**
     * @brief Получить весь временной ряд из буфера
     * 
     * @return std::pair<std::vector<Value>, std::vector<TimePoint>> 
     *         Пара векторов значений и соответствующих временных меток
     */
    std::pair<std::vector<Value>, std::vector<TimePoint>> getAll() const {
        return std::make_pair(m_valueBuffer.getAll(), m_timeBuffer.getAll());
    }

    /**
     * @brief Получить последние N точек временного ряда
     * 
     * @param n Количество последних точек для получения
     * @return std::pair<std::vector<Value>, std::vector<TimePoint>> 
     *         Пара векторов значений и соответствующих временных меток
     */
    std::pair<std::vector<Value>, std::vector<TimePoint>> getLast(std::size_t n) const {
        return std::make_pair(m_valueBuffer.getLast(n), m_timeBuffer.getLast(n));
    }

    /**
     * @brief Очистить буфер временного ряда
     */
    void clear() {
        m_valueBuffer.clear();
        m_timeBuffer.clear();
    }

    /**
     * @brief Получить текущий размер буфера
     * 
     * @return std::size_t Текущий размер буфера (количество точек временного ряда)
     */
    std::size_t size() const {
        return m_valueBuffer.size();
    }

    /**
     * @brief Получить емкость буфера
     * 
     * @return std::size_t Емкость буфера (максимальное количество точек временного ряда)
     */
    std::size_t capacity() const {
        return m_valueBuffer.capacity();
    }

    /**
     * @brief Проверить, пуст ли буфер
     * 
     * @return true если буфер пуст
     * @return false если буфер содержит точки временного ряда
     */
    bool empty() const {
        return m_valueBuffer.empty();
    }

    /**
     * @brief Проверить, полон ли буфер
     * 
     * @return true если буфер полон
     * @return false если в буфере есть свободное место
     */
    bool full() const {
        return m_valueBuffer.full();
    }

private:
    CircularBuffer<Value> m_valueBuffer;      ///< Буфер для хранения значений
    CircularBuffer<TimePoint> m_timeBuffer;   ///< Буфер для хранения временных меток
};

} // namespace storage
} // namespace rtems 