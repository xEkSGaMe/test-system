cmake_minimum_required(VERSION 3.16)
project(core-api CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(core-api
    src/main.cpp
    src/database/Database.cpp
    src/services/TestService.cpp
    src/services/QuestionService.cpp   # новый сервис
    src/services/AnswerService.cpp     # новый сервис
)

target_include_directories(core-api PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/database
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services
)

# Просто линкуем libpqxx и libpq
target_link_libraries(core-api PRIVATE pqxx pq)
