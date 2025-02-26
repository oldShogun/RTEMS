cmake_minimum_required(VERSION 3.15)
project(RTEMS VERSION 0.1.0 LANGUAGES CXX)

# Стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Опции компиляции
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Создаем список файлов исходного кода
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
)

# Создаем список заголовочных файлов
file(GLOB_RECURSE HEADERS
    "include/*.hpp"
)

# Создаем исполняемый файл
add_executable(rtems ${SOURCES} ${HEADERS})

# Указываем пути включения
target_include_directories(rtems PRIVATE ${PROJECT_SOURCE_DIR}/include)

# Проверка поддержки параллельной обработки
find_package(Threads REQUIRED)
target_link_libraries(rtems PRIVATE Threads::Threads)

# Настройка выходных путей
set_target_properties(rtems PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Создание директорий данных
add_custom_command(TARGET rtems POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bin/data
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bin/data/configs
)

# Копирование конфигурационных файлов (если будут добавлены)
# add_custom_command(TARGET rtems POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#     ${CMAKE_SOURCE_DIR}/data/configs
#     ${CMAKE_BINARY_DIR}/bin/data/configs
# )

# Опция для сборки тестов (для будущей разработки)
option(BUILD_TESTS "Build tests" OFF)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Вывод информации о конфигурации
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Project version: ${PROJECT_VERSION}") 