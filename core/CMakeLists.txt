cmake_minimum_required(VERSION 3.16)
project(core-api CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Поиск OpenSSL (обязателен для JWT)
find_package(OpenSSL REQUIRED)

add_executable(core-api
    src/main.cpp
    src/database/Database.cpp
    src/services/TestService.cpp
    src/services/QuestionService.cpp
    src/services/AnswerService.cpp
    src/services/AttemptUtils.cpp
)

target_include_directories(core-api PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/database
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services
    ${OPENSSL_INCLUDE_DIR}
)

# Линкуем всё вместе: pqxx, pq и OpenSSL
target_link_libraries(core-api PRIVATE 
    pqxx 
    pq 
    OpenSSL::Crypto
    OpenSSL::SSL
)