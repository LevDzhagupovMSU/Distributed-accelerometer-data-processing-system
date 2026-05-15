cmake_minimum_required(VERSION 3.16)
project(task2_level1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(COMMON_SOURCES
    src/logger.cpp
    src/node_a/serializer.cpp
    src/node_a/packet.cpp
)

add_executable(task2_server
    src/main.cpp
    src/server/server.cpp
    src/server/session.cpp
    src/node_a/client_a.cpp
    src/node_a/sensor_emulator.cpp
    src/node_b/client_b.cpp
    ${COMMON_SOURCES}
)

add_executable(task2_client_a
    src/main_client_a.cpp
    src/node_a/client_a.cpp
    src/node_a/sensor_emulator.cpp
    ${COMMON_SOURCES}
)

add_executable(task2_client_b
    src/main_client_b.cpp
    src/node_b/client_b.cpp
    ${COMMON_SOURCES}
)

target_include_directories(task2_server PRIVATE
    include
    include/node_a
    include/node_b
    include/server
)

target_include_directories(task2_client_a PRIVATE
    include
    include/node_a
    include/node_b
    include/server
)

target_include_directories(task2_client_b PRIVATE
    include
    include/node_a
    include/node_b
    include/server
)

target_link_libraries(task2_server
    boost_system
    pthread
)

target_link_libraries(task2_client_a
    boost_system
    pthread
)

target_link_libraries(task2_client_b
    boost_system
    pthread
)

find_package(nlohmann_json REQUIRED)
target_link_libraries(task2_server nlohmann_json::nlohmann_json)
target_link_libraries(task2_client_a nlohmann_json::nlohmann_json)
target_link_libraries(task2_client_b nlohmann_json::nlohmann_json)

enable_testing()
add_subdirectory(tests)