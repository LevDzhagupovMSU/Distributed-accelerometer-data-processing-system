# Подключаем Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
)
FetchContent_MakeAvailable(googletest)

# Собираем тесты
add_executable(run_unit_tests
    test_packet.cpp
    test_serializer.cpp
    test_sensor_emulator.cpp
    test_logger.cpp
    test_client_a.cpp
    test_client_b.cpp
    ${CMAKE_SOURCE_DIR}/src/node_a/packet.cpp
    ${CMAKE_SOURCE_DIR}/src/node_a/serializer.cpp
    ${CMAKE_SOURCE_DIR}/src/node_a/sensor_emulator.cpp
    ${CMAKE_SOURCE_DIR}/src/logger.cpp
    ${CMAKE_SOURCE_DIR}/src/node_a/client_a.cpp
    ${CMAKE_SOURCE_DIR}/src/node_b/client_b.cpp
)

target_include_directories(run_unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/node_a
    ${CMAKE_SOURCE_DIR}/include/node_b
    ${CMAKE_SOURCE_DIR}/include/server
)

target_link_libraries(run_unit_tests
    GTest::gtest
    GTest::gtest_main
    boost_system
    pthread
    nlohmann_json::nlohmann_json
)

add_test(NAME unit_tests COMMAND run_unit_tests)