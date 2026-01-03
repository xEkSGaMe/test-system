cmake_minimum_required(VERSION 3.16)
project(core-api CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Поиск OpenSSL (обязателен для JWT)
find_package(OpenSSL REQUIRED)

# Для nlohmann/json - если не найдет системный, будем использовать встроенный
# Сначала пробуем найти
find_package(nlohmann_json 3.10.5 QUIET)

add_executable(core-api
    src/main.cpp
    src/database/Database.cpp
    src/services/TestService.cpp
    src/services/QuestionService.cpp
    src/services/AnswerService.cpp
    src/services/AttemptUtils.cpp
    src/utils/HttpClient.cpp
)

target_include_directories(core-api PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/database
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
    ${OPENSSL_INCLUDE_DIR}
)

# Линкуем всё вместе: pqxx, pq и OpenSSL
target_link_libraries(core-api PRIVATE 
    pqxx 
    pq 
    OpenSSL::Crypto
    OpenSSL::SSL
)

# Если nlohmann/json не найден как пакет, добавим как интерфейсную библиотеку
if(NOT TARGET nlohmann_json::nlohmann_json)
    message(STATUS "nlohmann_json not found as system package, using local include")
    # Создаем интерфейсную библиотеку для json
    add_library(nlohmann_json INTERFACE)
    target_include_directories(nlohmann_json INTERFACE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(core-api PRIVATE nlohmann_json)
else()
    message(STATUS "Found nlohmann_json as system package")
    target_link_libraries(core-api PRIVATE nlohmann_json::nlohmann_json)
endif()

# Для jwt-cpp просто включаем заголовки
target_include_directories(core-api PRIVATE /usr/local/include)