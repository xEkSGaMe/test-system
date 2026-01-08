cmake_minimum_required(VERSION 3.16)
project(core-api CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Поиск зависимостей (как в вашем Dockerfile)
find_package(OpenSSL REQUIRED)
find_package(PostgreSQL REQUIRED)
# Библиотека pqxx часто ищется через pkg-config в Linux
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

add_executable(core-api
    src/main.cpp
    src/database/Database.cpp
    src/services/TestService.cpp
    src/services/QuestionService.cpp
    src/services/AnswerService.cpp
    src/services/AttemptUtils.cpp
    src/utils/HttpClient.cpp
)

# Указываем пути к инклудам (добавил PQXX)
target_include_directories(core-api PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/database
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
    ${OPENSSL_INCLUDE_DIR}
    ${PQXX_INCLUDE_DIRS}
    ${PostgreSQL_INCLUDE_DIRS}
)

# Линковка: сохраняем ваши зависимости + добавляем то, что нужно для БД
target_link_libraries(core-api PRIVATE 
    ${PQXX_LIBRARIES}
    ${PostgreSQL_LIBRARIES}
    OpenSSL::Crypto
    OpenSSL::SSL
    pthread
)

# nlohmann_json у вас в Dockerfile качается в /usr/local/include
# Поэтому просто разрешаем его находить там
target_include_directories(core-api PRIVATE /usr/local/include)