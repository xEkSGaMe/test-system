cmake_minimum_required(VERSION 3.10)
project(core-api)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Для cpp-httplib (header-only библиотека)
# Просто добавляем заголовочный файл

# Создаём исполняемый файл
add_executable(core-api src/main.cpp)

# Настраиваем свойства
target_compile_features(core-api PRIVATE cxx_std_17)

# Для Linux/macOS добавляем pthread
if(UNIX AND NOT APPLE)
    target_link_libraries(core-api pthread)
endif()

if(WIN32)
    target_link_libraries(core-api ws2_32 wsock32)
endif()
